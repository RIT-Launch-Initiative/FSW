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
    static CTask helloTask("Hello Task", 20, 512, K_MSEC(50));
    static CHelloTenant worldTenant("World");
    static CHelloTenant launchTenant("Launch");

    helloTask.AddTenant(worldTenant);
    helloTask.AddTenant(launchTenant);

    helloTask.Initialize();

    NRtos::AddTask(helloTask);
    NRtos::StartRtos();
    k_sleep(K_SECONDS(5));
    NRtos::StopRtos();

    return 0;
}
