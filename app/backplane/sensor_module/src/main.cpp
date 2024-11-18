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
    k_msleep(2000); // TODO: Fix race condition
    sensorModule.AddTenantsToTasks();
    sensorModule.AddTasksToRtos();
    sensorModule.SetupCallbacks();

    NRtos::StartRtos();

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(300));
    NRtos::StopRtos();
#endif

    return 0;
}

