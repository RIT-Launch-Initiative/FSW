/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "c_receiver_module.h"

#include <f_core/os/c_task.h>
#include <f_core/os/n_rtos.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

int main() {
#ifdef CONFIG_LICENSED_FREQUENCY
    LOG_INF("Receiver boot: 433 MHz build");
#else
    LOG_INF("Receiver boot: 915 MHz build");
#endif

    static CReceiverModule receiverModule{};
    receiverModule.AddTenantsToTasks();
    receiverModule.AddTasksToRtos();
    receiverModule.SetupCallbacks();

    k_sched_time_slice_set(1000, 14); // 1ms slice for priority 14
    k_sched_time_slice_set(5000, 15); // 5ms slice for priority 15
    NRtos::StartRtos();

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(300));
    NRtos::StopRtos();
#endif

    return 0;
}
