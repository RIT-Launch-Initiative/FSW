/*
* Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <f_core/utils/n_time_utils.h>
#include <f_core/device/c_rtc.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main() {
    CRtc rtc{*DEVICE_DT_GET(DT_ALIAS(rtc))};
    const char* sntpServerAddr = "10.2.1.1";

    if (NTimeUtils::SntpSynchronize(rtc, sntpServerAddr, 5)) {
        LOG_ERR("Failed to synchronize over SNTP");
    } else {
        LOG_INF("Time synchronized over NTP");
    }

    while (true) {
        rtc_time time{0};
        rtc.GetTime(time);

        LOG_INF("%d-%02d-%02d %02d:%02d:%02d", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
