/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "f_core/utils/n_time_utils.h"
#include <time.h>
#include <tuple>
#include <f_core/device/c_rtc.h>
#include <f_core/utils/c_soft_timer.h>

LOG_MODULE_REGISTER(NTimeUtils);

#ifdef CONFIG_SNTP
int NTimeUtils::SntpSynchronize(CRtc& rtc, const char* serverAddress, const int maxRetries) {
    int retryCount = 0;
    sntp_time ts{0};

    // Note this is a 100ms timeout. Zephyr does a poor job of documenting this.
    LOG_INF("Synchronizing time using NTP with server %s", serverAddress);
    while (sntp_simple(serverAddress, 1000, &ts) && retryCount < maxRetries) {
        k_sleep(K_SECONDS(1));
        retryCount++;
        LOG_ERR("Failed to synchronize time. Retrying (%d)", retryCount);
    }

    if (retryCount >= maxRetries) {
        LOG_ERR("Failed to synchronize time with server %s", serverAddress);
        return -1;
    }

    LOG_INF("SNTP synchronized with server %s", serverAddress);
    rtc.SetUnixTime(ts.seconds);

    return 0;
}

void NTimeUtils::SetupSntpSynchronizationCallback(CRtc& rtc, const char* serverAddress, const int maxRetries, int interval) {
    static auto sntpCallback = [](k_timer* timer) {
        auto data = static_cast<std::tuple<CRtc&, const char*, int>*>(k_timer_user_data_get(timer));
        CRtc& rtc = std::get<0>(*data); // Note that RTC drivers "should be" thread safe, so KISS and don't do any synchronization here
        const char* serverAddress = std::get<1>(*data);
        int maxRetries = std::get<2>(*data);

        NTimeUtils::SntpSynchronize(rtc, serverAddress, maxRetries);
    };

    static CSoftTimer timer{sntpCallback};
    if (timer.IsRunning()) {
        LOG_WRN("SNTP synchronization is already running!");
        return;
    }

    timer.SetUserData(new std::tuple<CRtc&, const char*, int>(rtc, serverAddress, maxRetries));
    timer.StartTimer(interval, 0);
}

#endif