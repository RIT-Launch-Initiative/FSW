/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <f_core/os/c_task.h>
#include <f_core/os/n_rtos.h>
#include <zephyr/drivers/gpio.h>

#ifndef CONFIG_RADIO_MODULE_RECEIVER
#include "c_radio_module.h"
#else
#include "c_receiver_module.h"
#endif

LOG_MODULE_REGISTER(main);

int main() {
#ifndef CONFIG_RADIO_MODULE_RECEIVER
    LOG_INF("Transmitter started");
    static CRadioModule radioModule{};
#else
    LOG_INF("Receiver started");
    static CReceiverModule radioModule{};
#endif
    radioModule.AddTenantsToTasks();
    radioModule.AddTasksToRtos();
    radioModule.SetupCallbacks();

    NRtos::StartRtos();
    k_sched_time_slice_set(1000, 14); // 1ms slice for priority 14
    k_sched_time_slice_set(5000, 15); // 5ms slice for priority 15

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(300));
    NRtos::StopRtos();
#endif

    return 0;
}

