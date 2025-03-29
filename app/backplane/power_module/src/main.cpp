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
#include <zephyr/net/sntp.h>
#include <arpa/inet.h>

LOG_MODULE_REGISTER(main);

int main() {
    static CPowerModule powerModule{};

    powerModule.AddTenantsToTasks();
    powerModule.AddTasksToRtos();
    powerModule.SetupCallbacks();

    // NRtos::StartRtos();

    // const device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
    // const device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
    // const char* sntpServerAddr = (CREATE_IP_ADDR(NNetworkDefs::RADIO_MODULE_IP_ADDR_BASE, 1, CONFIG_MODULE_ID)).c_str();
    // if (NTimeUtils::SntpSynchronize(*rtc, sntpServerAddr, 5)) {
    //     LOG_ERR("Failed to synchronize over SNTP");
    // } else {
    //     LOG_INF("Time synchronized over NTP");
    // }
    //
    // while (true) {
    //     rtc_time time{0};
    //     const device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
    //     rtc_get_time(rtc, &time);
    //
    //     LOG_INF("%d-%02d-%02d %02d:%02d:%02d", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
    //     k_sleep(K_SECONDS(2));
    // }

    struct sntp_ctx ctx;
    struct sockaddr_in addr;
    struct sntp_time sntp_time;
    int rv;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(123);

    net_addr_pton(AF_INET, "10.2.1.1", &addr.sin_addr);

    rv = sntp_init(&ctx, (struct sockaddr *) &addr,
               sizeof(struct sockaddr_in));
    if (rv < 0) {
        LOG_ERR("Failed to init SNTP IPv4 ctx: %d", rv);
    }


    while (true) {
        LOG_INF("Sending SNTP IPv4 request...");
        rv = sntp_query(&ctx, 4 * MSEC_PER_SEC, &sntp_time);
        if (rv < 0) {
            LOG_ERR("SNTP IPv4 request failed: %d", rv);
        } else {
            LOG_INF("status: %d", rv);
            LOG_INF("time since Epoch: high word: %u, low word: %u",
                (uint32_t)(sntp_time.seconds >> 32), (uint32_t)sntp_time.seconds);
        }

        k_msleep(5000);
    }
    return 0;
}

