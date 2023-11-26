/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef POWER_MODULE_NET_UTILS_H_
#define POWER_MODULE_NET_UTILS_H_

#include <stdint.h>
#include <stddef.h>

int init_eth_iface(void); 

int init_net_stack(void);

int send_udp_broadcast(const uint8_t *data, size_t data_len);

#endif
