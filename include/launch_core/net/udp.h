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
 * Send a UDP broadcast message
 * @param buff - Buffer of data to transmit
 * @param len - Size of the buffer
 * @param port - Port to transmit on
 * @return Zephyr status code
 */
int l_send_udp_broadcast(int sock, const uint8_t *buff, size_t len, uint16_t port);

/**
 * Polling receive function for UDP
 * @param data - Buffer to receive data into
 * @param data_len - Size of the buffer
 * @param port - Port the data was received on
 * @return Zephyr status code
 */
int l_receive_udp_poll(int sock, const uint8_t *buff, size_t len, uint16_t port);

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

void l_receive_multicast_packets(int port, uint8_t *buffer, size_t buffer_size);

int l_udp_receive(int port, uint8_t *buffer, size_t buffer_size);


#endif // L_UDP_UTILS_H_