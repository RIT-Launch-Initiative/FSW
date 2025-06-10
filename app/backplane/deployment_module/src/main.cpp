/*
* Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "c_deployment_module.h"

#include <f_core/os/n_rtos.h>

LOG_MODULE_REGISTER(main);

int main() {
    // static CDeploymentModule deploymentModule{};
    //
    // deploymentModule.AddTenantsToTasks();
    // deploymentModule.AddTasksToRtos();
    //
    // NRtos::StartRtos();

    const gpio_dt_spec pyro0 = GPIO_DT_SPEC_GET(DT_ALIAS(pyro_ctrl_0), gpios);
    const gpio_dt_spec pyro1 = GPIO_DT_SPEC_GET(DT_ALIAS(pyro_ctrl_1), gpios);
    const gpio_dt_spec pyro2 = GPIO_DT_SPEC_GET(DT_ALIAS(pyro_ctrl_2), gpios);
    const gpio_dt_spec pyro3 = GPIO_DT_SPEC_GET(DT_ALIAS(pyro_ctrl_3), gpios);

    const gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
    const gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
    const gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
    const gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);


    k_sleep(K_SECONDS(10));
    LOG_INF("Blowing charges");

    gpio_pin_set_dt(&pyro0, 1);
    gpio_pin_set_dt(&led0, 1);
    gpio_pin_set_dt(&pyro1, 1);
    gpio_pin_set_dt(&led1, 1);
    gpio_pin_set_dt(&pyro2, 1);
    gpio_pin_set_dt(&led2, 1);
    gpio_pin_set_dt(&pyro3, 1);
    gpio_pin_set_dt(&led3, 1);

    while (true) {
        gpio_pin_toggle_dt(&led0);
        k_msleep(1000);
    }

    // k_sleep(K_SECONDS(10));
    // LOG_INF("Turning off charge enable");
    // gpio_pin_set_dt(&pyro0, 0);
    // gpio_pin_set_dt(&led0, 0);
    // gpio_pin_set_dt(&pyro1, 0);
    // gpio_pin_set_dt(&led1, 0);
    // gpio_pin_set_dt(&pyro2, 0);
    // gpio_pin_set_dt(&led2, 0);
    // gpio_pin_set_dt(&pyro3, 0);
    // gpio_pin_set_dt(&led3, 0);


#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(900));
    NRtos::StopRtos();
    deploymentModule.Cleanup();
#endif

    return 0;
}
