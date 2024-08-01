/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/os/n_rtos.h>
#include <f_core/os/c_task.h>
#include "c_publisher.h"
#include "c_receiver.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

int main() {
    // Tenants and Tasks are expected to be statically allocated, since exiting main will call their destructors
    // Calling the tasks destructors will stop the tasks and free up the allocated stacks
    // This can lead to a double free too if StopRtos is called before main exits
    static CReceiver printWorldTenant("World");
    static CReceiver printLaunchTenant("Launch");
    static CTask printTask("Print Task", 15, 512, 1000);

    printTask.AddTenant(printWorldTenant);
    printTask.AddTenant(printLaunchTenant);
    NRtos::AddTask(printTask);

    int count = 0;
    static CPublisher counterTenantOne("Counter 1", &count);
    static CPublisher counterTenantTwo("Counter 2", &count);
    static CTask counterTask("Counter Task", 15, 512, 80);

    counterTask.AddTenant(counterTenantOne);
    counterTask.AddTenant(counterTenantTwo);
    NRtos::AddTask(counterTask);

    NRtos::StartRtos();
    k_msleep(5000);
    NRtos::StopRtos();

    return 0;
}
