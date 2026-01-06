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

static const spi_config slaveConfig = {
    .frequency = 4000000,
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
    SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_OP_MODE_SLAVE,
    .slave = 0,
};

struct SpiArmCommandPacket {
    uint8_t commandNumber;
    float shoulderYaw;
    float shoulderPitch;
    float elbowAngle;
    float wristAngle;
    uint8_t takePicture;
} __attribute__((packed));

struct SpiStatusPacket {
    uint8_t faultStatus;
    float m1CommandedSpeed;
    float m2CommandedSpeed;
    float m3CommandedSpeed;
    float servoCommandedAngle;
    float shoulderYaw;
    float shoulderPitch;
    float elbowAngle;
    float shoulderYawLimitSwitch;
    float temp1;
    float temp2;
} __attribute__((packed));

static uint8_t slaveTxBuffer[sizeof(SpiStatusPacket)];
static uint8_t slaveRxBuffer[sizeof(SpiArmCommandPacket)];

static K_THREAD_STACK_DEFINE(spiSlaveStack, 4096);
static k_thread spiSlaveThread;

static void spiSlaveInit(void) {
    slaveDev = DEVICE_DT_GET(SPI_SLAVE_NODE);
    if (!device_is_ready(slaveDev)) {
        LOG_ERR("SPI slave device not ready!");
        return;
    }
    LOG_INF("SPI slave initialized on SPI1");
}

static void printReceivedCommand(const SpiArmCommandPacket& cmd) {
    LOG_INF("=== Received Command from Master ===");
    LOG_INF("Command Number: %u", cmd.commandNumber);
    // Cast to doubles for printing to avoid float format issues
    LOG_INF("Shoulder Yaw: %.2f", static_cast<double>(cmd.shoulderYaw));
    LOG_INF("Shoulder Pitch: %.2f", static_cast<double>(cmd.shoulderPitch));
    LOG_INF("Elbow Angle: %.2f", static_cast<double>(cmd.elbowAngle));
    LOG_INF("Wrist Angle: %.2f", static_cast<double>(cmd.wristAngle));
    LOG_INF("Take Picture: %u", cmd.takePicture);
    LOG_INF("=====================================");
}

static void prepareStatusResponse(SpiStatusPacket& status) {
    memset(&status, 0, sizeof(SpiStatusPacket));
    status.faultStatus = 0;
    status.m1CommandedSpeed = 0.0f;
    status.m2CommandedSpeed = 0.0f;
    status.m3CommandedSpeed = 0.0f;
    status.servoCommandedAngle = 0.0f;
    status.shoulderYaw = 0.0f;
    status.shoulderPitch = 0.0f;
    status.elbowAngle = 0.0f;
    status.shoulderYawLimitSwitch = 0.0f;
    status.temp1 = 25.0f;
    status.temp2 = 25.0f;
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
        .buf = slaveRxBuffer,
        .len = sizeof(slaveRxBuffer),
    };
    const spi_buf_set rxBuffSet = {
        .buffers = &rxBuff,
        .count = 1
    };

    if (int error = spi_transceive(slaveDev, &slaveConfig, &txBuffSet, &rxBuffSet); error != 0) {
        LOG_ERR("SPI slave transceive error: %i", error);
        return error;
    }

    const SpiArmCommandPacket& cmd = *reinterpret_cast<const SpiArmCommandPacket*>(slaveRxBuffer);
    printReceivedCommand(cmd);
    LOG_INF("Status response sent back to master");
    return 0;
}

static void spiSlaveThreadEntry(void*, void*, void*) {
    LOG_INF("SPI Slave thread started");

    SpiStatusPacket status{0};
    prepareStatusResponse(status);
    memcpy(slaveTxBuffer, &status, sizeof(SpiStatusPacket));

    while (true) {
        if (spiSlaveListen() == 0) {
            prepareStatusResponse(status);
            memcpy(slaveTxBuffer, &status, sizeof(SpiStatusPacket));
        }
        k_msleep(100);
    }
}

int main() {
    spiSlaveInit();
    LOG_INF("SPI Slave listening for commands from master");

    k_thread_create(&spiSlaveThread, spiSlaveStack,
                    K_THREAD_STACK_SIZEOF(spiSlaveStack),
                    spiSlaveThreadEntry, NULL, NULL, NULL,
                    5, 0, K_NO_WAIT);

    k_thread_name_set(&spiSlaveThread, "spi_slave");

    while (true) {
        k_msleep(1000);
    }

    return 0;
}
