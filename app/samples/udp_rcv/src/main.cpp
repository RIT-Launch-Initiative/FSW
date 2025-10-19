/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// F-Core Includes
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/net/network/c_ipv4.h>
#include <f_core/messaging/c_msgq_message_port.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

K_MSGQ_DEFINE(receiveQueue, sizeof(const char[32]), 10, 4);

int main() {
    static constexpr char ipAddrStr[] = "10.0.0.0";
    static constexpr int udpPort = 10000;
    static auto messagePort = CMsgqMessagePort<char[32]>(receiveQueue);


    while (true) {

    }


    return 0;
}
