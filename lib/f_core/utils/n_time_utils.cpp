/*
 * Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "f_core/utils/n_time_utils.h"
#include <stdint.h>

#ifdef CONFIG_SNTP

// Days in each month (non-leap year)
static constexpr int daysInMonth[12] = {
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};

static bool isLeapYear(int year) {
    year += 1900;
    return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
}

static int calculateWeekday(uint64_t daysSinceEpoch) {
    // Jan 1, 1970 was a Thursday (4)
    return (static_cast<int>(daysSinceEpoch) + 4) % 7;
}

void NTimeUtils::SntpToRtcTime(const sntp_time& sntp, rtc_time& rtc) {
    // Imagine gmtime causes linker issues with Zephyr C++ libs
    // so you need to hand-roll your own implementation :/
    uint64_t seconds = sntp.seconds;
    uint64_t days = seconds / 86400;
    uint64_t rem = seconds % 86400;

    rtc.tm_hour = rem / 3600;
    rem %= 3600;
    rtc.tm_min = rem / 60;
    rtc.tm_sec = rem % 60;

    int year = 70; // Since 1900
    int dayCount = static_cast<int>(days);

    while (true) {
        int daysInYear = isLeapYear(year) ? 366 : 365;
        if (dayCount < daysInYear) break;
        dayCount -= daysInYear;
        year++;
    }

    rtc.tm_year = year;
    rtc.tm_yday = dayCount;

    int month = 0;
    while (month < 12) {
        int dim = daysInMonth[month];
        if (month == 1 && isLeapYear(year)) dim += 1;
        if (dayCount < dim) break;
        dayCount -= dim;
        month++;
    }

    rtc.tm_mon = month;
    rtc.tm_mday = dayCount + 1;
    rtc.tm_wday = calculateWeekday(days);
    rtc.tm_isdst = -1;

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

    if (retryCount >= maxRetries) {
        return -1;
    }

    SntpToRtcTime(ts, rtcTime);
    rtc_set_time(&rtc, &rtcTime);

    return 0;
}

#endif
