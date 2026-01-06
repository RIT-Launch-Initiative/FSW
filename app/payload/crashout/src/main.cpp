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

LOG_MODULE_REGISTER(main);

#define SPI_SLAVE_NODE DT_NODELABEL(spi1)

const struct device *spi_slave_dev;
static struct k_poll_signal spi_slave_done_sig = K_POLL_SIGNAL_INITIALIZER(spi_slave_done_sig);

static const struct spi_config spi_slave_cfg = {
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

static uint8_t slave_tx_buffer[sizeof(struct SpiStatusPacket)];
static uint8_t slave_rx_buffer[sizeof(struct SpiArmCommandPacket)];

static void spi_slave_init(void) {
    spi_slave_dev = DEVICE_DT_GET(SPI_SLAVE_NODE);
    if (!device_is_ready(spi_slave_dev)) {
        LOG_ERR("SPI slave device not ready!");
        return;
    }
    LOG_INF("SPI slave initialized on SPI1");
}

static void print_received_command(const struct SpiArmCommandPacket *cmd) {

}

static void prepare_status_response(struct SpiStatusPacket *status) {

}

static int spi_slave_listen(void) {

}

int main() {
    LOG_INF("Application started");
    
    spi_slave_init();
    LOG_INF("SPI Slave listening for commands from master");

    SpiStatusPacket status;
    prepare_status_response(&status);
    memcpy(slave_tx_buffer, &status, sizeof(SpiStatusPacket));

    while (true) {

        k_msleep(100);
    }

    return 0;
}
