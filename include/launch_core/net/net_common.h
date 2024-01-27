/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Common utility functions for dealing with Zephyr networking libraries
 */

#ifndef L_NET_UTILS_H_
#define L_NET_UTILS_H_

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

#endif // L_NET_UTILS_H_