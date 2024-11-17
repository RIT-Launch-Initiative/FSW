/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "c_radio_module.h"

#include <f_core/os/c_task.h>
#include <f_core/os/n_rtos.h>
#include <zephyr/drivers/gpio.h>

int main() {
    static CRadioModule radioModule{};

    radioModule.AddTenantsToTasks();
    radioModule.AddTasksToRtos();
    radioModule.SetupCallbacks();

    NRtos::StartRtos();

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(300));
    NRtos::StopRtos();
#endif

    return 0;
}

