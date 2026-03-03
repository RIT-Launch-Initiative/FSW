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
    .frequency = 10000000,
    .slave = 0, // address
};

int main(void) {
    // k_poll_signal_init(&spi_periph_done_sig);
    // k_poll_signal
    if (!device_is_ready(spi_periph_dev)) {
        printk("SPI periph device not ready!\n");
    }

    // int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    // if (ret < 0) {
        // printk("Error %d: failed to configure led pin \n", ret);
        // return 0;
    // }
    // printk("Setup LED\n");



    int i = 0;
    while (i < 1) {
        i++;
        printk("Waiting for gpio\n");
        k_msleep(1000);
    }

    // return 0;



    while (1) {
        // k_poll(events, 1, K_FOREVER);
        // k_sem_take(&spi_sem, K_FOREVER);
        // gpio_pin_set_dt(&led, true);
        // ret = gpio_pin_set_dt(&led, true);
        // 0xa3 10100011
        // 0x6c 01011100
        uint8_t out_buf[4] = {0x00, 0x12, 0x34, 0x00};
        
        struct spi_buf tx_buf = {
            .buf = out_buf,
            .len = sizeof(out_buf),
        };
        struct spi_buf_set tx_bufs = {.buffers = &tx_buf, .count = 1};

        uint8_t in_buf[4] = {0};
        struct spi_buf rx_buf = {
            .buf = in_buf,
            .len = sizeof(in_buf),
        };
        struct spi_buf_set rx_bufs = {.buffers = &rx_buf, .count = 1};
        printk("waiting\n");

        int sret = spi_transceive(spi_periph_dev, &spi_periph_cfg, &tx_bufs, &rx_bufs);


        // int ret = spi_read(spi_periph_dev, &spi_periph_cfg, &rx_bufs);
        // gpio_pin_set_dt(&led, true);

        // int sret = spi_write(spi_periph_dev, &spi_periph_cfg, &tx_bufs);

        if (sret < 0) {
            printk("f\n");
        } else {
            printk("Uptime: %lld\n", k_uptime_get());
            printk("STXed: %02x %02x %02x %02x\n", out_buf[0], out_buf[1], out_buf[2], out_buf[3]);
            printk("SRXed: %02x %02x %02x %02x\n", in_buf[0], in_buf[1], in_buf[2], in_buf[3]);
        }
        k_usleep(0);

        // if (sret < 0){
            // printk("txf\n");
        // } else {
            // printk("txed\n");
        // }
        // k_msleep(10);
        // gpio_pin_set_dt(&led, false);
        // k_msleep(10);
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
