/*
 * Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>

#include <string.h>
#include <stdint.h>

LOG_MODULE_REGISTER(main);

#define SPI_SLAVE_NODE DT_NODELABEL(spi1)

const device* slaveDev;
static k_poll_signal spi_slave_done_sig = K_POLL_SIGNAL_INITIALIZER(spi_slave_done_sig);

static const spi_config spi_slave_cfg = {
    .frequency = 4000000,
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
    SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_OP_MODE_SLAVE,
    .slave = 0,
};

struct SpiArmCommandPacket {
    uint8_t command_number;
    float shoulder_yaw;
    float shoulder_pitch;
    float elbow_angle;
    float wrist_angle;
    uint8_t take_picture;
} __attribute__((packed));

struct SpiStatusPacket {
    uint8_t fault_status;
    float m1_commanded_speed;
    float m2_commanded_speed;
    float m3_commanded_speed;
    float servo_commanded_angle;
    float shoulder_yaw;
    float shoulder_pitch;
    float elbow_angle;
    float shoulder_yaw_limit_switch;
    float temp1;
    float temp2;
} __attribute__((packed));

static uint8_t slaveTxBuffer[sizeof(SpiStatusPacket)];
static uint8_t slave_rx_buffer[sizeof(SpiArmCommandPacket)];

static void spiSlaveInit(void) {
    slaveDev = DEVICE_DT_GET(SPI_SLAVE_NODE);
    if (!device_is_ready(slaveDev)) {
        LOG_ERR("SPI slave device not ready!");
        return;
    }
    LOG_INF("SPI slave initialized on SPI1");
}

static void printReceivedCommand(const SpiArmCommandPacket* cmd) {
    LOG_INF("=== Received Command from Master ===");
    LOG_INF("Command Number: %u", cmd->command_number);
    LOG_INF("Shoulder Yaw: %.2f", cmd->shoulder_yaw);
    LOG_INF("Shoulder Pitch: %.2f", cmd->shoulder_pitch);
    LOG_INF("Elbow Angle: %.2f", cmd->elbow_angle);
    LOG_INF("Wrist Angle: %.2f", cmd->wrist_angle);
    LOG_INF("Take Picture: %u", cmd->take_picture);
    LOG_INF("=====================================");
}

static void prepareStatusResponse(SpiStatusPacket* status) {
    memset(status, 0, sizeof(SpiStatusPacket));
    status->fault_status = 0;
    status->m1_commanded_speed = 0.0f;
    status->m2_commanded_speed = 0.0f;
    status->m3_commanded_speed = 0.0f;
    status->servo_commanded_angle = 0.0f;
    status->shoulder_yaw = 0.0f;
    status->shoulder_pitch = 0.0f;
    status->elbow_angle = 0.0f;
    status->shoulder_yaw_limit_switch = 0.0f;
    status->temp1 = 25.0f;
    status->temp2 = 25.0f;
}

static int spiSlaveListen() {
    const spi_buf txBuff = {
        .buf = slaveTxBuffer,
        .len = sizeof(slaveTxBuffer)
    };
    const spi_buf_set txBuffSet = {
        .buffers = &txBuff,
        .count = 1
    };

    spi_buf rxBuff = {
        .buf = slave_rx_buffer,
        .len = sizeof(slave_rx_buffer),
    };
    const spi_buf_set rxBuffSet = {
        .buffers = &rxBuff,
        .count = 1
    };

    k_poll_signal_reset(&spi_slave_done_sig);

    if (int error = spi_transceive_signal(slaveDev, &spi_slave_cfg, &txBuffSet, &rxBuffSet, &spi_slave_done_sig); error != 0) {
        LOG_ERR("SPI slave transceive error: %i", error);
        return error;
    }

    unsigned int signaled = 0;
    int result = 0;
    k_poll_signal_check(&spi_slave_done_sig, &signaled, &result);
    if (signaled != 0) {
        SpiArmCommandPacket const* cmd = (struct SpiArmCommandPacket*)slave_rx_buffer;
        printReceivedCommand(cmd);
        LOG_INF("Status response sent back to master");
        return 0;
    }
    return -1;
}

int main() {
    LOG_INF("Application started");

    spiSlaveInit();
    LOG_INF("SPI Slave listening for commands from master");

    SpiStatusPacket status{0};
    prepareStatusResponse(&status);
    memcpy(slaveTxBuffer, &status, sizeof(SpiStatusPacket));

    while (true) {
        if (spiSlaveListen() == 0) {
            prepareStatusResponse(&status);
            memcpy(slaveTxBuffer, &status, sizeof(SpiStatusPacket));
        }
        k_msleep(100);
    }

    return 0;
}
