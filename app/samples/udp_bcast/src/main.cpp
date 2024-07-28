/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// F-Core Includes
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/net/network/c_ipv4.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

int main() {
    CIPv4 ip("10.0.0.0");
    ip.Initialize();

    CUdpSocket udp(ip, 10000, 10000);

    while (true) {
        udp.TransmitAsynchronous("Hello, Launch!", 14);
        k_msleep(1000);
    }


    return 0;
}
