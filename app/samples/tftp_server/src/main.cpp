/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/os/n_rtos.h>
#include <f_core/net/application/c_tftp_server_tenant.h>

#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(main);

int main() {
    CTftpServerTenant *tftpServer = CTftpServerTenant::getInstance(CIPv4("10.0.0.1"));

    return 0;
}
