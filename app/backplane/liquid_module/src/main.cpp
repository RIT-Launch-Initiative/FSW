/*
 * Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

#define AD4112_NODE DT_NODELABEL(ad4112)
#define MAX31856_NODE DT_NODELABEL(max31856)

static const struct spi_dt_spec ad4112_dev = SPI_DT_SPEC_GET(
    AD4112_NODE, 
    SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA, 
    0
);
static const struct spi_dt_spec max31856_dev = SPI_DT_SPEC_GET(
    MAX31856_NODE,
    SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA,
    0
);

LOG_MODULE_REGISTER(main);

// Note: Use Functions that end in DT

int main() {
    LOG_INF("Application started");

    if(!spi_is_ready_dt(&ad4112_dev)){
        printk("No PT");
        return -1;
    }
    if(!spi_is_ready_dt(&max31856_dev)){
        printk("No TC");
    }
    
    while (true) {
        k_msleep(1000);
        // Wait this shits kinda hard, but also I'm goated so...
    }

    return 0;
}
