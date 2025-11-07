/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "c_sensor_module.h"

#include <f_core/os/n_rtos.h>
#include <zephyr/shell/shell.h>

static CSensorModule sensorModule{};
int main() {

    sensorModule.AddTenantsToTasks();
    sensorModule.AddTasksToRtos();
    sensorModule.SetupCallbacks();

    NRtos::StartRtos();

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(900));
    NRtos::StopRtos();
    sensorModule.Cleanup();
#endif

    return 0;
}

int cmd_doboost(const struct shell* shell, size_t argc, char** argv) {
    sensorModule.Controller().SubmitEvent(Sources::LowGImu, Events::Boost);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(control_subcmds, SHELL_CMD(boost, NULL, "Send LSM Boost", cmd_doboost),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(control, &control_subcmds, "Control Commands", NULL);
