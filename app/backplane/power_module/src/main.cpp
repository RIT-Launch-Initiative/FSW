/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "c_power_module.h"

#include <f_core/os/c_task.h>
#include <f_core/os/n_rtos.h>
#include <n_autocoder_network_defs.h>
#include <f_core/utils/n_time_utils.h>

LOG_MODULE_REGISTER(main);

int main() {
    static CPowerModule powerModule{};

    powerModule.AddTenantsToTasks();
    powerModule.AddTasksToRtos();
    powerModule.SetupCallbacks();

    NRtos::StartRtos();

    // const device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
    const device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
    const char* sntpServerAddr = (CREATE_IP_ADDR(NNetworkDefs::RADIO_MODULE_IP_ADDR_BASE, 1, CONFIG_MODULE_ID)).c_str();
    if (NTimeUtils::SntpSynchronize(*rtc, sntpServerAddr, 5)) {
        LOG_ERR("Failed to synchronize over SNTP");
    } else {
        LOG_INF("Time synchronized over NTP");
    }

    while (true) {
        rtc_time time{0};
        const device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
        rtc_get_time(rtc, &time);

        LOG_INF("%d-%02d-%02d %02d:%02d:%02d", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
        k_sleep(K_SECONDS(2));
    }

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(300));
    NRtos::StopRtos();
    powerModule.Cleanup();
    k_sleep(K_FOREVER);
#endif


    return 0;
}

