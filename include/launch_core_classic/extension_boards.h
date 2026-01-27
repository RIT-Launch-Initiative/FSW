/*
 * Copyright (c) 2024 Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Define extension board constants and functionality for interactions
 */

#pragma once
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>

/********** GENERAL **********/

typedef enum {
    NO_EXTENSION_BOARD = -1,
    POTATO_EXTENSION_BOARD_ID = 1,
} l_extension_board_id_t;

typedef void (*l_extension_board_data_handler_t)(void *data, size_t len);

/**
 * Used by extension boards to identify themselves to the parent module
 * @param dev - Pointer to UART device
 * @param base_port - Base port of the parent module
 * @param timeout_ms - Time before arbitration times out. 0 for no timeout
 * @return ID of the extension board. -1 if no board is found
 */
l_extension_board_id_t find_extension_board(const struct device *dev, uint16_t base_port, uint32_t timeout_ms);

/**
 * Used by extension boards to identify themselves to the parent module
 * @param id - ID of the extension board
 * @param timeout_ms - Time before arbitration times out. 0 for no timeout
 * @return Base port of the parent module. -1 if we are an orphan :(
 */
int initiate_arbitration(l_extension_board_id_t id, uint32_t timeout_ms);

/********** POTATO **********/

/**
 * Helper function for parent module to receive POTATO data
 * @param data - Pointer to data buffer
 * @param len - Size of data buffer
 */
void receive_potato_data(void *data, size_t len);


