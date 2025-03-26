/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "f_core/utils/n_time_utils.h"
#include <time.h>

#ifdef CONFIG_SNTP
void NTimeUtils::SntpToRtcTime(const sntp_time& sntp, rtc_time& rtc) {
    auto rawTime = static_cast<time_t>(sntp.seconds);

    tm* tmPtr = gmtime(&rawTime);
    if (!tmPtr) {
        memset(&rtc, 0, sizeof(rtc));
        return;
    }

    rtc.tm_sec = tmPtr->tm_sec;
    rtc.tm_min = tmPtr->tm_min;
    rtc.tm_hour = tmPtr->tm_hour;
    rtc.tm_mday = tmPtr->tm_mday;
    rtc.tm_mon = tmPtr->tm_mon;
    rtc.tm_year = tmPtr->tm_year;
    rtc.tm_wday = tmPtr->tm_wday;
    rtc.tm_yday = tmPtr->tm_yday;
    rtc.tm_isdst = tmPtr->tm_isdst;
    rtc.tm_nsec = static_cast<int>((static_cast<uint64_t>(sntp.fraction) * 1000000000ULL) >> 32);
}   


int NTimeUtils::SntpSynchronize(const device& rtc, const char* serverAddress, const int maxRetries) {
    int retryCount = 0;
    sntp_time ts{0};
    rtc_time rtcTime{0};

    // Note this is a 100ms timeout. Zephyr does a poor job of documenting this.
    while (sntp_simple(serverAddress, 100, &ts) && retryCount < maxRetries) {
        k_sleep(K_SECONDS(1));
        retryCount++;
    }

    if (retryCount >= 5) {
        return -1;
    }

    SntpToRtcTime(ts, rtcTime);
    rtc_set_time(&rtc, &rtcTime);

    return 0;
}
#endif