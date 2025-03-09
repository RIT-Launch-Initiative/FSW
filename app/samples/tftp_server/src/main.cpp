/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/os/n_rtos.h>
#include <f_core/net/application/c_tftp_server_tenant.h>
#include <f_core/net/application/c_udp_broadcast_tenant.h>

#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(main);

int main() {
    CIPv4 ip("10.0.0.1");
    CTftpServerTenant *tftpServer = CTftpServerTenant::getInstance(ip);
    CUdpSocket udp(ip, 10000, 10000); // Just broadcast timestamps to ensure we can run both on the same thread

    uint32_t uptime = k_uptime_get_32();
    while (true) {
        uptime = k_uptime_get_32();
        tftpServer->Run();
        udp.TransmitAsynchronous(&uptime, sizeof(uint32_t));
    }

    return 0;
}
