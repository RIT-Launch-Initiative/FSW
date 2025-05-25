/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/os/n_rtos.h>
#include <f_core/os/c_task.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

int main() {
    NRtos::StartRtos();
    k_msleep(5000);
    NRtos::StopRtos();

    return 0;
}
