/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include "structure.h"

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 500

/* The devicetree node identifier for the "led0" alias. */

// const struct device *spi_periph_dev = DEVICE_DT_GET(DT_NODELABEL(arduino_spi));

#define SPI_OPERATION (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_TRANSFER_MSB)
const struct spi_dt_spec spidt = SPI_DT_SPEC_GET(DT_ALIAS(mc), SPI_OPERATION);

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});

static struct exposed_state st = {
    .uptime_ms = 1234,
    .status = 0,
    .system_fault = 0,
    .motor_fault = 0,
    .commanded_joint1_angle = 0, // motor: Shoulder yaw
    .commanded_joint2_angle = 0, // motor: Shoulder pitch
    .commanded_joint3_angle = 0, // motor: Elbow pitch
    .commanded_joint4_angle = 0, // servo: Wrist pitch
    .base_accel_x = 0,
    .base_accel_y = 0,
    .base_accel_z = 0,
    .current_joint1_angle = 0, // motor: Shoulder yaw
    .current_joint2_angle = 0, // motor: Shoulder pitch
    .current_joint3_angle = 0, // motor: Elbow pitch
    .accel1_x = 0,
    .accel1_y = 0,
    .accel1_z = 0,
    .accel2_y = 0,
    .accel2_x = 0,
    .accel2_z = 0,
    .m1_voltage = 0,
    .m1_current = 0,
    .m2_voltage = 0,
    .m2_current = 0,
    .m3_voltage = 0,
    .m3_current = 0,
    .uptime_of_last_base_accel_write = 0,
};

int main(void) {

    int ret;

    if (!device_is_ready(spidt.bus)) {
        printk("Spi not ready\n");
        return 0;
    }

    if (!gpio_is_ready_dt(&button)) {
        printk("Error: button device %s is not ready\n", button.port->name);
        return 0;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
        printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
        return 0;
    }

    uint8_t tx[1] = {0xa3};
    
    struct spi_buf tx_buf = {.buf = &st, .len = sizeof(tx)};
    struct spi_buf rx_buf = {.buf = &st, .len = sizeof(struct exposed_state)};

    struct spi_buf_set tx_bufs = {.buffers = &tx_buf, .count = 1};
    struct spi_buf_set rx_bufs = {.buffers = &rx_buf, .count = 1}; // &rx_buf

    while (1) {
        int ret = 0;

        while (1) {
            int val = gpio_pin_get_dt(&button);
            if (val) {
                break;
            }
            k_msleep(10);
        }
        printk("transceiveing\n");

        // printk("transceiveing\n");
        // ret = spi_write_dt(&spidt, &tx_bufs);
        // k_usleep(400);
        // int ret2 = spi_read_dt(&spidt, &rx_bufs);
        uint8_t b = k_uptime_get() % 256;
        tx[0] = b;
        tx[1] = b;
        tx[2] = b;
        tx[3] = 0;
        ret = spi_transceive_dt(&spidt, &tx_bufs, &rx_bufs);
        if (ret != 0) {
            printk("Ret: %d\n", ret);
        } else {
            printk("Uptime: %lld\n", k_uptime_get());
            printk("Txed: %02x %02x %02x %02x\n", tx[0], tx[1], tx[2], tx[3]);
            // printk("Rxed: %02x %02x %02x %02x\n", rx[0], rx[1], rx[2], rx[3]);
            printk("SUptime: %d\n", st.uptime_ms);
            printk("S: %d\n", st.accel1_x);
        }
        // if (ret2 != 0) {
        // printk("Ret2: %d\n", ret);
        // } else {
        // }
        // ret = gpio_pin_set_dt(&led, false);

        k_msleep(50);
    }
    return 0;
}
