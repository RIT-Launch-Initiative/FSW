/*
* Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/device/c_rtc.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

void printTime(const tm &time) {
    LOG_INF("%02d-%02d-%04d %02d:%02d:%02d", time.tm_mon + 1, time.tm_mday, time.tm_year + 1900, time.tm_hour, time.tm_min, time.tm_sec);
}


int main() {
    const device *rtcDev = DEVICE_DT_GET(DT_ALIAS(rtc));
    CRtc rtc{*rtcDev};

    // Intentionally use tm instead of rtc_time so tm overload calls underlying rtc_time allowing us to test both
    tm timeToSet{
        .tm_sec = 0,
        .tm_min = 0,
        .tm_hour = 0,
        .tm_mday = 1,
        .tm_mon = 0,
        .tm_year = 2000 - 1900,
    };

    tm currentTime{0};

    // Initial time 1-1-1970 00:00:00
    LOG_INF("Setting time to 1970 using UNIX");
    rtc.SetUnixTime(0);
    rtc.GetTime(currentTime);
    printTime(currentTime);

    // Ensure we are still at the same time
    LOG_INF("Travel to Y2K using tm");
    rtc.SetTime(timeToSet);




    return 0;
}
