/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "f_core/utils/n_time_utils.h"
#include <tuple>
#include <f_core/device/c_rtc.h>
#include <f_core/utils/c_soft_timer.h>

LOG_MODULE_REGISTER(NTimeUtils);

#ifdef CONFIG_SNTP
int NTimeUtils::SntpSynchronize(CRtc& rtc, const char* serverAddress, const int maxRetries, const k_timeout_t retryDelay) {
    int retryCount = 0;
    sntp_time ts{0};

    // Note this is a 100ms timeout. Zephyr does a poor job of documenting this.
    LOG_INF("Synchronizing time using NTP with server %s", serverAddress);
    while (sntp_simple(serverAddress, 10, &ts) && retryCount < maxRetries) {
        k_sleep(retryDelay);
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

void NTimeUtils::SetupSntpSynchronizationCallback(CRtc& rtc, const int interval, const char* serverAddress, const int maxRetries, const k_timeout_t retryDelay) {
    // TODO: Timeout in sntp_simple gets things stuck so don't do use this function (yet)
    // Maybe we just set up another tenant or create a new thread that we can suspend and resume using the timer
    LOG_ERR("DO NOT USE. BUGGED");
    k_oops();


    static std::tuple<CRtc&, const char*, int, const k_timeout_t> sntpData{rtc, serverAddress, maxRetries, retryDelay};
    static auto sntpCallback = [](k_timer* timer) {
        auto data = static_cast<std::tuple<CRtc&, const char*, int, k_timeout_t>*>(k_timer_user_data_get(timer));
        CRtc& rtc = std::get<0>(*data); // Note that RTC drivers "should be" thread safe, so KISS and don't do any synchronization here
        const char* serverAddress = std::get<1>(*data);
        int maxRetries = std::get<2>(*data);
        k_timeout_t retryDelay = std::get<3>(*data);

        SntpSynchronize(rtc, serverAddress, maxRetries, retryDelay);
    };

    static CSoftTimer timer{sntpCallback};
    if (timer.IsRunning()) {
        LOG_WRN("SNTP synchronization is already running!");
        return;
    }

    timer.SetUserData(&sntpData);
    timer.StartTimer(interval, 0);
}

#endif