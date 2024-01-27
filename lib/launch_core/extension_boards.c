/*
 * Copyright (c) 2024 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdint.h>

#include <launch_core/extension_boards.h>

#include <zephyr/drivers/uart.h>

static l_extension_board_id_t check_extension_board_id(uint32_t received_byte) {
    switch (received_byte) {
        case POTATO_EXTENSION_BOARD_ID:
            return POTATO_EXTENSION_BOARD_ID;
        default:
            return NO_EXTENSION_BOARD;
    }
}

l_extension_board_id_t find_extension_board(const struct device *dev, uint16_t base_port, uint32_t timeout_ms) {
    uint32_t start_time = k_uptime_get_32();
    uint32_t current_time = start_time;
    uint32_t elapsed_time = 0;
    uint32_t received_byte = 0;

    if (timeout_ms == 0) {
        timeout_ms = K_FOREVER;
    }

    while (elapsed_time < timeout_ms) {
        uart_poll_in(dev, received_byte, 1); // TODO: Maybe use SLIP
        if (check_extension_board_id(received_byte) != NO_EXTENSION_BOARD) {
            LOG_INF("Extension board found with ID: %d", received_byte);
            // TODO: Send an acknowledgement
            uart_tx(dev, received_byte, 1, 0); // TODO: Maybe use SLIP
            return received_byte;
        }


        current_time = k_uptime_get_32();
        elapsed_time = current_time - start_time;
    }

    LOG_ERR("Failed to determine extension board ID");
    return NO_EXTENSION_BOARD;
}


int initiate_arbitration(l_extension_board_id_t id, uint32_t timeout_ms) {
    uint32_t start_time = k_uptime_get_32();
    uint32_t current_time = start_time;
    uint32_t elapsed_time = 0;
    uint32_t received_byte = 0;

    if (timeout_ms == 0) {
        timeout_ms = K_FOREVER;
    }

    while (elapsed_time < timeout_ms) {
        uart_tx(dev, received_byte, 1, 0);
        uart_poll_in(dev, received_byte, 1); // TODO: Use a callback instead
        if (received_byte != 0) {
            LOG_INF("Extension board found by module with ID: %d", received_byte);
            return received_byte;
        }

        current_time = k_uptime_get_32();
        elapsed_time = current_time - start_time;
    }

    return -1;
}


void receive_potato_data(void *data, size_t len) {
    // TODO: Implement when we know what data to send
}

#endif
