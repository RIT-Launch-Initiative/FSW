/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 500

/* The devicetree node identifier for the "led0" alias. */

const struct device *spi_periph_dev = DEVICE_DT_GET(DT_NODELABEL(arduino_spi));

static const struct spi_config spi_periph_cfg = {
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_OP_MODE_MASTER,
    .frequency = 375000,
    .slave = 0,
};

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});

int main(void) {

    int ret;
    bool led_state = true;

    if (!device_is_ready(spi_periph_dev)) {
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

    uint8_t tx[4] = {0xa3, 0x00, 0x00, 0x00};
    uint8_t rx[4] = {0};
    struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
    struct spi_buf rx_buf = {.buf = rx, .len = sizeof(rx)};

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
        ret = spi_transceive(spi_periph_dev, &spi_periph_cfg, &tx_bufs, &rx_bufs);
        if (ret != 0) {
            printk("Ret: %d\n", ret);
        }
        // ret = gpio_pin_set_dt(&led, false);

        k_msleep(50);
    }
    return 0;
}
