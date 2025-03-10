/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <f_core/net/application/c_tftp_server_tenant.h>
#include <f_core/net/application/c_udp_broadcast_tenant.h>
#include <f_core/os/c_file.h>
#include <f_core/os/n_rtos.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main() {
    CIPv4 ip("10.0.0.1");
    CTftpServerTenant *tftpServer = CTftpServerTenant::getInstance(ip);
    CUdpSocket udp(ip, 10000, 10000); // Just broadcast timestamps to ensure we can run both on the same thread

    LOG_INF("Creating file");

    CFile file("/lfs/test.txt", FS_O_READ | FS_O_WRITE | FS_O_CREATE);
    if (file.GetStatus() < 0) {
        LOG_ERR("Error opening file");
        return -1;
    }

    int ret = file.Write("Hello, world!", 13);
    LOG_INF("Wrote to file %d", ret);



    uint32_t uptime = k_uptime_get_32();
    while (true) {
        uptime = k_uptime_get_32();
        tftpServer->Run();
        udp.TransmitAsynchronous(&uptime, sizeof(uint32_t));
        k_msleep(100);
    }

    return 0;
}
