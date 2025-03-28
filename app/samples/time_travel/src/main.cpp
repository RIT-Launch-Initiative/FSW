/*
* Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/device/c_rtc.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

void printTime(const tm &time, time_t unixTime) {
    LOG_INF("\t%02d-%02d-%04d %02d:%02d:%02d", time.tm_mon + 1, time.tm_mday, time.tm_year + 1900, time.tm_hour, time.tm_min, time.tm_sec);

#ifdef CONFIG_RTC_STM32
    LOG_INF("\t%lld", unixTime);
#else
    LOG_INF("\t%d", unixTime);
#endif
}


int main() {
    const device *rtcDev = DEVICE_DT_GET(DT_ALIAS(rtc));
    CRtc rtc{*rtcDev};

    // Intentionally use tm instead of rtc_time since overloaded fn
    // calls underlying rtc_time implementation, allowing us to test both
    tm timeToSet{
        .tm_sec = 0,
        .tm_min = 0,
        .tm_hour = 0,
        .tm_mday = 1,
        .tm_mon = 0,
        .tm_year = 2000 - 1900,
    };

    tm currentTime{0};
    time_t unixTime = 0;

    // Initial time 1-1-1970 00:00:00
#ifndef CONFIG_RTC_STM32
    LOG_INF("Setting time to 1970 using UNIX");

    rtc.SetUnixTime(0);
    rtc.GetTime(currentTime);
    rtc.GetUnixTime(unixTime);

    printTime(currentTime, unixTime);

    // Travel to Y2K
    LOG_INF("Travel to Y2K using tm");

    rtc.SetTime(timeToSet);
    rtc.GetTime(currentTime);
    rtc.GetUnixTime(unixTime);

    printTime(currentTime, unixTime);
#endif

    // Travel to 2025
    LOG_INF("Travel to 2025 using UNIX");

    rtc.SetUnixTime(1735689600);
    rtc.GetTime(currentTime);
    rtc.GetUnixTime(unixTime);

    printTime(currentTime, unixTime);

    // Christmas
    LOG_INF("Christmas comes earlier every year...");
    timeToSet.tm_year = 2025 - 1900;
    timeToSet.tm_mon = 11;
    timeToSet.tm_mday = 25;
    timeToSet.tm_hour = 8;
    timeToSet.tm_min = 0;
    timeToSet.tm_sec = 0;

    rtc.SetTime(timeToSet);
    rtc.GetTime(currentTime);
    rtc.GetUnixTime(unixTime);

    printTime(currentTime, unixTime);

    // Wait for 5 seconds
    LOG_INF("Does sleeping count as time travel?");
    k_msleep(5040); // 40 extra ms to guarantee we are past the second mark
    rtc.GetTime(currentTime);
    rtc.GetUnixTime(unixTime);
    printTime(currentTime, unixTime);

    return 0;
}
