/*
* Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "c_deployment_module.h"

#include <f_core/os/n_rtos.h>

LOG_MODULE_REGISTER(main);

int main() {
    static CDeploymentModule deploymentModule{};

    deploymentModule.AddTenantsToTasks();
    deploymentModule.AddTasksToRtos();
    deploymentModule.SetupCallbacks();

    NRtos::StartRtos();

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(900));
    NRtos::StopRtos();
    deploymentModule.Cleanup();
#endif

    return 0;
}
