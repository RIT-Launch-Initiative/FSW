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
    k_sched_time_slice_set(1000, 10); // 1ms slice for priorities 0-10
    k_sched_time_slice_set(5000, 15); // 5ms slice for priorities 11-15

    while (true) {
        rtc_time time{0};
        const device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
        rtc_get_time(rtc, &time);

        LOG_INF("%d-%02d-%02d %02d:%02d:%02d", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
        k_sleep(K_SECONDS(1));
    }

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(300));
    NRtos::StopRtos();
#endif

    return 0;
}

