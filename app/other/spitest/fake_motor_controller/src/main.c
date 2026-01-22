/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>

// #include <zephyr/kernel.h>

const struct device *spi_periph_dev = DEVICE_DT_GET(DT_NODELABEL(arduino_spi));

// static struct k_poll_signal spi_periph_done_sig = K_POLL_SIGNAL_INITIALIZER(spi_periph_done_sig);

// struct k_poll_event events[1] = {
// K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &spi_periph_done_sig),
// };

K_SEM_DEFINE(spi_sem, 0, 1);

static const struct spi_config spi_periph_cfg = {
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_OP_MODE_SLAVE,
    .frequency = 375000,
    .slave = 0,
};

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_NODELABEL(cs_spi), gpios);
static struct gpio_callback button_cb_data;

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    // gpio_pin_set_dt(&led, true);
    // k_sem_give(&spi_sem);
    // gpio_pin_set_dt(&led, false);

    // if (ret < 0) {
    // printk("Error reading spi: %d\n", ret);
    // } else {
    // printk("Read %d bytes from spi: ", ret);
    // for (int i = 0; i < ret; i++) {
    // printk("%02x ", in_buf[i]);
    // }
    // printk("\n");
    // }
}

int main(void) {
    // k_poll_signal_init(&spi_periph_done_sig);
    // k_poll_signal
    if (!device_is_ready(spi_periph_dev)) {
        printk("SPI periph device not ready!\n");
    }

    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: failed to configure led pin \n", ret);
        return 0;
    }
    printk("Setup LED\n");

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
        printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
        return 0;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin);
        return 0;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);
    printk("Set up button at %s pin %d\n", button.port->name, button.pin);

    printk("Waiting for gpio\n");
    while (1) {
        // k_poll(events, 1, K_FOREVER);
        // k_sem_take(&spi_sem, K_FOREVER);
        // gpio_pin_set_dt(&led, true);
        // ret = gpio_pin_set_dt(&led, true);
        uint8_t in_buf[1];
        struct spi_buf rx_buf = {
            .buf = in_buf,
            .len = sizeof(in_buf),
        };
        struct spi_buf_set rx_bufs = {.buffers = &rx_buf, .count = 1};
        printk("waiting\n");
        int ret = spi_read(spi_periph_dev, &spi_periph_cfg, &rx_bufs);
        gpio_pin_set_dt(&led, true);

        if (ret < 0) {
            printk("f\n");
        } else {
            printk("b: %02x\n", in_buf[0]);
        }
        gpio_pin_set_dt(&led, false);
        // k_busy_wait(k_ms_to_ticks_near32(10));
        // k_poll_signal_reset(&spi_periph_done_sig);
        // printk("tick\n");

        // int ret = spi_read(spi_periph_dev, &spi_periph_cfg, &rx_bufs);
        // if (ret < 0) {
        //     printk("Error reading spi: %d\n", ret);
        // } else {
        //     printk("Read %d bytes from spi: ", ret);
        //     for (int i = 0; i < ret; i++) {
        //         printk("%02x ", in_buf[i]);
        //     }
        //     printk("\n");
        // }
        // spi_read_dt()
        // k_msleep(1000);
    }
    return 0;
}
