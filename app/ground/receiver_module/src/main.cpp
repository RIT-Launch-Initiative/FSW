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
#ifdef CONFIG_ARCH_POSIX
#include <signal.h>
#include <nsi_main.h>
#endif

LOG_MODULE_REGISTER(main);

#ifdef CONFIG_ARCH_POSIX
static volatile sig_atomic_t shutdownRequested = 0;

static void NativeSimSignalHandler(int) { shutdownRequested = 1; }
#endif

int main() {
    LOG_INF("Receiver starting");
    static CReceiverModule receiverModule{};
    receiverModule.AddTenantsToTasks();
    receiverModule.AddTasksToRtos();
    receiverModule.SetupCallbacks();

    k_sched_time_slice_set(1000, 14); // 1ms slice for priority 14
    k_sched_time_slice_set(5000, 15); // 5ms slice for priority 15
    NRtos::StartRtos();

#ifdef CONFIG_ARCH_POSIX
    signal(SIGINT, NativeSimSignalHandler);
    signal(SIGTERM, NativeSimSignalHandler);

    while (!shutdownRequested) {
        k_sleep(K_MSEC(100));
    }

    NRtos::StopRtos();
    nsi_exit(0);
#endif

    return 0;
}
