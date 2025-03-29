/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "c_power_module.h"

#include <f_core/os/c_task.h>
#include <f_core/os/n_rtos.h>
#include <n_autocoder_network_defs.h>
#include <zephyr/net/sntp.h>
#include <arpa/inet.h>


LOG_MODULE_REGISTER(main);

int main() {
    static CPowerModule powerModule{};

    powerModule.AddTenantsToTasks();
    powerModule.AddTasksToRtos();
    powerModule.SetupCallbacks();

    NRtos::StartRtos();

    return 0;
}

