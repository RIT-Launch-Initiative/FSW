/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Utility functions for dealing with Zephyr networking libraries
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

/********** GENERAL **********/


/**
 * Create a string representation of an IP address
 * @param ip_str - Pointer to a buffer to store the string
 * @param a - First octet
 * @param b - Second octet
 * @param c - Third octet
 * @param d - Fourth octet
 * @return Number of characters written to the buffer or negative error code
 */
int l_create_ip_str(char *ip_str, int a, int b, int c, int d);

/**
 * Create a string representation of an IP address with the default network ID
 * @param ip_str - Pointer to a buffer to store the string
 * @param c - Third octet
 * @param d - Fourth octet
 * @return Number of characters written to the buffer or negative error code
 */
int l_create_ip_str_default_net_id(char *ip_str, int c, int d);

/********** UDP **********/
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
int l_add_port_handler(uint16_t port, l_udp_port_handler_t *handler);

/**
 * Remove a port handler
 * @param port - Port to remove
 * @return Zephyr status code
 */
int l_remove_port_handler(uint16_t port);


#endif
