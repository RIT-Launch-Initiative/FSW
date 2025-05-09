/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// F-Core Includes
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/application/c_udp_broadcast_tenant.h>
#include <f_core/messaging/c_msgq_message_port.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);
static constexpr char broadcastStr[] = "Hello, Launch!";
static constexpr char secondBroadcastStr[] = "Hello, World!";

static constexpr size_t BROADCAST_STRLEN = sizeof(broadcastStr);
static constexpr size_t SECOND_BROADCAST_STRLEN = sizeof(secondBroadcastStr);

K_MSGQ_DEFINE(broadcastQueue, sizeof(const char[BROADCAST_STRLEN]), 10, 4);
K_MSGQ_DEFINE(secondBroadcastQueue, sizeof(const char[SECOND_BROADCAST_STRLEN]), 10, 4);

int main() {
    static constexpr char ipAddrStr[] = "10.0.0.0";
    static constexpr int udpPort = 10000;
    static constexpr int secondBroadcasterSrcPort = 11000;
    static constexpr int secondBroadcasterDstPort = 12001;

    auto messagePort = CMsgqMessagePort<char[BROADCAST_STRLEN]>(broadcastQueue);
    CUdpBroadcastTenant broadcaster("Broadcast Tenant", ipAddrStr, udpPort, udpPort, messagePort);

    auto secondMessagePort = CMsgqMessagePort<char[SECOND_BROADCAST_STRLEN]>(secondBroadcastQueue);
    CUdpBroadcastTenant secondBroadcaster("Second Broadcast Tenant", ipAddrStr, secondBroadcasterSrcPort, secondBroadcasterDstPort, secondMessagePort);

    while (true) {
        messagePort.Send(broadcastStr, K_NO_WAIT);
        secondMessagePort.Send(secondBroadcastStr, K_NO_WAIT);
        broadcaster.Run();
        secondBroadcaster.Run();

        LOG_INF("Transmitted");
        k_sleep(K_SECONDS(1));
    }

    // The code below is effectively the same

    // CIPv4 ip("10.0.0.0");
    // CUdpSocket udp(ip, 10000, 10000);
    //
    // while (true) {
    //     udp.TransmitSynchronous("Hello, Launch!", 14);
    //     LOG_INF("Transmitted");
    //     k_sleep(K_SECONDS(1));
    // }

    return 0;
}
