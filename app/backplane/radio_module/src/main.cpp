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

void initRtcTo1970() {
    rtc_time time = {
        .tm_sec = 0,
        .tm_min = 0,
        .tm_hour = 0,
        .tm_mday = 1,
        .tm_mon = 0,
        .tm_year = 70,
        .tm_wday = 4,
        .tm_yday = 0,
        .tm_isdst = 0,
        .tm_nsec = 0,
    };
    rtc_set_time(DEVICE_DT_GET(DT_ALIAS(rtc)), &time);
}

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
    k_sched_time_slice_set(5000, 15);

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(300));
    NRtos::StopRtos();
#endif

    return 0;
}

