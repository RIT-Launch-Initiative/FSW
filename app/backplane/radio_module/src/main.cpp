/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "c_radio_module.h"

#include <f_core/os/c_task.h>
#include <f_core/os/n_rtos.h>

LOG_MODULE_REGISTER(main);

int main() {
#ifdef CONFIG_LICENSED_FREQUENCY
    LOG_INF("Radio module boot: 433 MHz build, callsign=%s", CONFIG_RADIO_MODULE_CALLSIGN);
#else
    LOG_INF("Radio module boot: 915 MHz build, callsign=%s", CONFIG_RADIO_MODULE_CALLSIGN);
#endif
    static CRadioModule radioModule{};

    radioModule.AddTenantsToTasks();
    radioModule.AddTasksToRtos();
    radioModule.SetupCallbacks();

    k_sched_time_slice_set(1000, 14); // 1ms slice for priority 14
    k_sched_time_slice_set(5000, 15); // 5ms slice for priority 15
    NRtos::StartRtos();

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(300));
    NRtos::StopRtos();
#endif

    return 0;
}
