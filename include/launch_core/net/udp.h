/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Utility functions for handling UDP networking
 */

#ifndef L_UDP_UTILS_H_
#define L_UDP_UTILS_H_

#include <stdint.h>
#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/net_event.h>

typedef void (*l_udp_handler_t)(uint8_t *data, size_t data_len);

typedef struct {
    uint16_t port;
    l_udp_handler_t handler;
} l_udp_port_handler_t;

/**
 * Initialize a UDP networking stack
 * @param ip - IP address to bind to
 * @return Zephyr status code
 */
int l_init_udp_net_stack(const char *ip);

int l_init_udp_socket(const char *ip, uint16_t port);

int l_deinit_udp_socket(int sock);

/**
 * Configure a receive timeout for a socket
 * @param sock - Socket to configure
 * @param timeout - Timeout in milliseconds
 * @return
 */
int l_set_socket_rx_timeout(int sock, int timeout);

/**
 * Send a UDP broadcast message
 * @param buff - Buffer of data to transmit
 * @param len - Size of the buffer
 * @param port - Port to transmit on
 * @return Zephyr status code
 */
int l_send_udp_broadcast(int sock, const uint8_t *buff, size_t len, uint16_t port);

/**
 * Receive function for UDP meant to run in its own thread (blocking function)
 * @param sock - Initialized socket to receive data on
 * @param buff - Buffer to store received data
 * @param len - Size of the buffer
 * @return Zephyr status code
 */
int l_receive_udp(int sock, const uint8_t *buff, size_t len);

/**
 * Add a function for handling data from a specific port
 * @param port - Port to handle
 * @param handler - Function to handle data
 * @return Zephyr status code
 */
int l_add_port_handler(uint16_t port, l_udp_port_handler_t *handler);

/**
 * Remove a port handler
 * @param port - Port to remove
 * @return Zephyr status code
 */
int l_remove_port_handler(uint16_t port);



#endif // L_UDP_UTILS_H_