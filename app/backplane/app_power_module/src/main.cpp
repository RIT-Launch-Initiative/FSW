/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "c_power_module_configuration.h"

// F-Core Includes
#include <f_core/os/n_rtos.h>

int main() {
    static CPowerModuleConfiguration configuration{};

    configuration.SetupTasks();
    configuration.SetupInterprocessCommunication();

    NRtos::StartRtos();

    return 0;
}

