/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "c_sensor_module.h"

#include <f_core/os/c_task.h>
#include <f_core/os/n_rtos.h>

int main() {
    static CSensorModule sensorModule{};

    sensorModule.AddTenantsToTasks();
    sensorModule.AddTasksToRtos();
    sensorModule.SetupCallbacks();

    NRtos::StartRtos();

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(900));
    NRtos::StopRtos();
    sensorModule.Cleanup();
    // k_sleep(K_FOREVER);
#endif

    return 0;
}
