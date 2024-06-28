/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/os/n_rtos.h>
#include <f_core/os/c_task.h>
#include "c_hello_tenant.h"
#include "c_print_count.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

int main() {
    CTask printTask("Print Task", 15, 128, 1000);
    CTask counterTask("Counter Task", 13, 512, 100);
    CHelloTenant printWorldTenant("World");
    CHelloTenant printLaunchTenant("Launch");

    int count = 0;
    CPrintCount counterTenantOne("Counter 1", &count);
    CPrintCount counterTenantTwo("Counter 2", &count);

    printTask.AddTenant(printWorldTenant);
    printTask.AddTenant(printLaunchTenant);

    counterTask.AddTenant(counterTenantOne);
    counterTask.AddTenant(counterTenantTwo);

    NRtos::AddTask(printTask);
    NRtos::AddTask(counterTask);
    NRtos::StartRtos();
    k_msleep(5000);
    NRtos::StopRtos();

    return 0;
}
