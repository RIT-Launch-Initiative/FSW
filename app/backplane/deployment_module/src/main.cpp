/*
* Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "c_deployment_module.h"

#include <f_core/os/n_rtos.h>
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
    static CDeploymentModule deploymentModule{};

    deploymentModule.AddTenantsToTasks();
    deploymentModule.AddTasksToRtos();
    deploymentModule.SetupCallbacks();

    NRtos::StartRtos();

#ifdef CONFIG_ARCH_POSIX
    signal(SIGINT, NativeSimSignalHandler);
    signal(SIGTERM, NativeSimSignalHandler);

    while (!shutdownRequested) {
        k_sleep(K_MSEC(100));
    }

    NRtos::StopRtos();
    deploymentModule.Cleanup();
    nsi_exit(0);
#endif

    return 0;
}
