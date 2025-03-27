/*
* Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/device/c_rtc.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

void printTime(const rtc_time &time) {
    LOG_INF("%02d-%02d-%04d %02d:%02d", time.tm_mon + 1, time.tm_mday, time.tm_year + 1900, time.tm_hour, time.tm_min);
}


int main() {
    const device *rtcDev = DEVICE_DT_GET(DT_ALIAS(rtc));
    CRtc rtc{*rtcDev};

    rtc.SetUnixTime(1743106890);

    rtc_time rtcTime{0};
    rtc.GetTime(rtcTime);
    printTime(rtcTime);



    return 0;
}
