/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "c_power_module.h"

#include <f_core/os/n_rtos.h>
#include <f_core/utils/n_bootcount.h>

LOG_MODULE_REGISTER(main);

int main() {
    NBootCount::IncrementBootCount();

    static CPowerModule powerModule{};

    powerModule.AddTenantsToTasks();
    powerModule.AddTasksToRtos();
    powerModule.SetupCallbacks();

    NRtos::StartRtos();

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(300));
    NRtos::StopRtos();
    powerModule.Cleanup();
    k_sleep(K_FOREVER);
#endif

    return 0;
}
