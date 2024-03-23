/*
 * Copyright (c) 2023 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Common utility functions for dealing with Zephyr networking libraries
 */

#ifndef L_NET_UTILS_H_
#define L_NET_UTILS_H_

#include <stdint.h>

// IPv4 Class A
#define CLASS_A_NETMASK "10.0.0.0"

#define CLASS_A_IP(network_octet, module_id, revision_number, board_number) \
    #network_octet "." #module_id "." #revision_number "." #board_number

// Class A Backplane Network
#define BACKPLANE_IP(module_id, revision_number, board_number) \
    CLASS_A_IP(10, module_id, revision_number, board_number)

#define MAX_IP_ADDRESS_STR_LEN 16

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

#endif // L_NET_UTILS_H_