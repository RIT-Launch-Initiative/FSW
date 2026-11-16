/*
 * Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>

#define AD4112_NODE DT_NODELABEL(ad4112)
static const struct spi_dt_spec ad4112_dev = SPI_DT_SPEC_GET(
    AD4112_NODE, 
    SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA, 
    0
);


LOG_MODULE_REGISTER(main);

int main() {


    LOG_INF("Application started");
    
    while (true) {
        k_msleep(1000);
        // Wait this shits kinda hard, but also I'm goated so...
    }

    return 0;
}
