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

int l_init_udp_net_stack(const char *ip);

int l_send_udp_broadcast(const uint8_t *data, size_t data_len, uint16_t port);

#endif
