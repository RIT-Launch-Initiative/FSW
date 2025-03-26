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

    const device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
    // const char* sntpServerAddr = (CREATE_IP_ADDR(NNetworkDefs::POWER_MODULE_IP_ADDR_BASE, 2, CONFIG_MODULE_ID)).c_str();
    if (NTimeUtils::SntpSynchronize(*rtc, "10.2.1.1", 5)) {
        LOG_ERR("Failed to synchronize over SNTP");
    }

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(10));
    NRtos::StopRtos();
    powerModule.Cleanup();
    k_sleep(K_FOREVER);
#endif


    return 0;
}

