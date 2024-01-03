/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef POWER_MODULE_NET_UTILS_H_
#define POWER_MODULE_NET_UTILS_H_

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialize the ethernet interface
 *
 * @return int 0 if successful, negative error code otherwise
 */
int init_eth_iface(void);

/**
 * @brief Initialize the network stack
 * @details More speicifically, this sets up the IP address of the ethernet device
 * @return int
 */
int init_net_stack(void);

/**
 * @brief Broadcast a UDP packet over ethernet
 *
 * @param data the data to send
 * @param data_len the size of the data to send
 * @param port the port number to send the data to
 * @return int 0 if successful, negative error code otherwise
 */
int send_udp_broadcast(const uint8_t *data, size_t data_len, uint16_t port);

#endif
