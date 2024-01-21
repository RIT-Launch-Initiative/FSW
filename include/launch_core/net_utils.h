/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef POWER_MODULE_NET_UTILS_H_
#define POWER_MODULE_NET_UTILS_H_

#include <stdint.h>
#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_event.h>


/********** UDP **********/
typedef struct {
    uint16_t port;
    void (*handler)(uint8_t *data, size_t data_len);
} l_udp_port_handler_t;

/**
 * Initialize a UDP networking stack
 * @param ip - IP address to bind to
 * @return Zephyr status code
 */
int l_init_udp_net_stack(const char *ip);

/**
 * Send a UDP broadcast message
 * @param buff - Buffer of data to transmit
 * @param len - Size of the buffer
 * @param port - Port to transmit on
 * @return Zephyr status code
 */
int l_send_udp_broadcast(const uint8_t *data, size_t data_len, uint16_t port);

/**
 * Receive UDP callback
 * @param dev - Network device
 * @param packet - Packet received
 * @param status - Status of the packet
 * @return Zephyr status code
 */
int l_receive_udp_callback(const struct device *dev, struct net_pkt *packet, int status);

/**
 * Add a function for handling data from a specific port
 * @param port - Port to handle
 * @param handler - Function to handle data
 * @return Zephyr status code
 */
int l_add_port_handler(uint16_t port, void (*handler)(uint8_t *data, size_t data_len));

/**
 * Remove a port handler
 * @param port - Port to remove
 * @return Zephyr status code
 */
int l_remove_port_handler(uint16_t port);


#endif
