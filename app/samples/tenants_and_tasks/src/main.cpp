/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/os/n_rtos.h>
#include <f_core/os/c_task.h>
#include "c_hello_tenant.h"

int main() {
    CHelloTenant worldTenant("World");
    CHelloTenant launchTenant("Launch");

    launchTenant.Run();
    worldTenant.Run();

    return 0;
}
