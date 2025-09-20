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
    CTftpServerTenant *tftpServer = CTftpServerTenant::GetInstance(ip);

    while (true) {
        tftpServer->Run();
        k_msleep(100);
    }

    return 0;
}
