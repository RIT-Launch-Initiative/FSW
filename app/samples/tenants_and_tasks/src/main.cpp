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
    CTask printTask("Print Task", 15);
    CTask incrementTask("Incrementor Task", 15);
    CHelloTenant printWorldTenant("World");
    CHelloTenant printLaunchTenant("Launch");

    printTask.AddTenant(printWorldTenant);
    printTask.AddTenant(printLaunchTenant);

    printTask.Initialize();
    printTask.Initialize();

    NRtos::AddTask(printTask);
    NRtos::AddTask(incrementTask);
    NRtos::StartRtos();
    k_msleep(1000);
    NRtos::StopRtos();

    return 0;
}
