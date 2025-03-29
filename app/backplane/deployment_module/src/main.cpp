/*
* Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "c_deployment_module.h"

#include <f_core/os/n_rtos.h>

LOG_MODULE_REGISTER(main);
struct PyroTrio {
    CGpio sense;
    CGpio ctrl;
    CGpio debugLed;
};
int main() {
    // static CDeploymentModule deploymentModule{};
    //
    // deploymentModule.AddTenantsToTasks();
    // deploymentModule.AddTasksToRtos();
    // deploymentModule.SetupCallbacks();

    // NRtos::StartRtos();
    LOG_INF("HELLO WORLD");

    std::array<PyroTrio, 4> pyroTrios{
        PyroTrio{
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_sns_0), gpios)),
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_ctrl_0), gpios)),
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios))
        },
        PyroTrio{
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_sns_1), gpios)),
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_ctrl_1), gpios)),
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios))
        },
        PyroTrio{
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_sns_2), gpios)),
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_ctrl_2), gpios)),
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios))
        },
        PyroTrio{
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_sns_3), gpios)),
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_ctrl_3), gpios)),
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios))
        },
    };

    for (int i = 0; i < 5; i++) {
        LOG_INF("Deploying in %d seconds", 5 - i);
        k_sleep(K_SECONDS(1));
    }
    int count = 0;
    for (auto& [sense, ctrl, led] : pyroTrios) {

        // if (sense.GetPin() == 1) {
            ctrl.SetPin(1);
            led.SetPin(1);
            LOG_INF("Deployed charge %d", count);
        // } else {
            LOG_INF("Did not detect continuity on %d", count);
        // }

        count++;
    }

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(900));
    NRtos::StopRtos();
    deploymentModule.Cleanup();
#endif

    return 0;
}
