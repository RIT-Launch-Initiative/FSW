/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/os/n_rtos.h>
#include <f_core/os/c_task.h>
#include "c_hello_tenant.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

int main() {
    printk("Hello, World");
    CHelloTenant worldTenant("World");
    CHelloTenant launchTenant("Launch");

    for (int i = 0; i < 10; i++) {
        launchTenant.Run();
        printk("\n");
        worldTenant.Run();
        printk("\n");
    }

    return 0;
}
