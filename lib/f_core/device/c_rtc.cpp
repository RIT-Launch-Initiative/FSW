#include "f_core/device/c_rtc.h"

#include <time.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/rtc.h>

LOG_MODULE_REGISTER(CRtc);

CRtc::CRtc(const device& dev) : rtc(dev) {
    rtc_time rtcTime{0};
    rtcTime.tm_year = 100;
    rtcTime.tm_mday = 1;

    if (GetTime(rtcTime) == -ENODATA) {
        LOG_INF("RTC not initialized, setting to default time");
        SetTime(rtcTime);
    }

    if (int ret = GetTime(rtcTime); ret < 0) {
        LOG_ERR("Failed to get RTC time during initialization: %d", ret);
    } else {
        LOG_INF("RTC initialized to %04d-%02d-%02d %02d:%02d:%02d", rtcTime.tm_year + 1900,
                rtcTime.tm_mon + 1, rtcTime.tm_mday, rtcTime.tm_hour, rtcTime.tm_min, rtcTime.tm_sec);
    }
}

static bool IsLeapYear(int year) {
    // full year (i.e. 2025), not year-1900
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int DaysInMonth(const int year, const int month) {
    static const int daysInMonth[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };
    if (month == 1 && IsLeapYear(year)) {
        return 29;
    }
    return daysInMonth[month];
}

static uint64_t ComputeUnixMillis(const rtc_time& rtcTime) {
    int year = rtcTime.tm_year + 1900;
    int month = rtcTime.tm_mon;
    int day = rtcTime.tm_mday;

    uint64_t days = 0; // Count total days since epoch

    // Years
    for (int y = 1970; y < year; ++y) {
        days += IsLeapYear(y) ? 366 : 365;
    }

    // Months
    for (int m = 0; m < month; ++m) {
        days += DaysInMonth(year, m);
    }

    // Days (tm_mday is 1-based)
    days += day - 1;

    // Convert to seconds
    uint64_t seconds = days * 86400ull
                     + rtcTime.tm_hour * 3600ull
                     + rtcTime.tm_min * 60ull
                     + rtcTime.tm_sec;

    // Convert to milliseconds
    return seconds * 1000ull + rtcTime.tm_nsec / 1000000;
}



int CRtc::GetTime(rtc_time& time) {
    if (int ret = rtc_get_time(&rtc, &time); ret < 0) {
        // LOG_ERR("Failed to get RTC time: %d", ret);
        return ret;
    }
    return 0;
}

int CRtc::GetTime(tm& time) {
    return GetTime(reinterpret_cast<rtc_time&>(time));
}

int CRtc::GetMillisTime(uint32_t& millis) {
    if (int ret = GetMillisTime(reinterpret_cast<uint64_t&>(millis)); ret < 0) {
        return ret;
    }

    return 0;
}

int CRtc::GetMillisTime(uint64_t& millis) {
    rtc_time rtcTime{};
    if (int ret = GetTime(rtcTime); ret < 0) {
        return ret;
    }

    millis = ComputeUnixMillis(rtcTime);
    return 0;
}

int CRtc::GetUnixTime(time_t& unixTimestamp) {
    tm time{0};
    if (int ret = GetTime(time); ret < 0) {
        return ret;
    }

    unixTimestamp = mktime(&time);
    return 0;
}

int CRtc::GetUnixTime(uint32_t& unixTimestamp) {
    tm time{0};
    if (int ret = GetTime(time); ret < 0) {
        return ret;
    }

    unixTimestamp = static_cast<uint32_t>(mktime(&time));
    return 0;
}


int CRtc::SetTime(rtc_time& rtcTime) {
#ifdef CONFIG_RTC_STM32
    if (rtcTime.tm_year < 100) {
        LOG_WRN("STM32 does not support years before 2000. This will most likely result in an EINVAL when setting RTC");
    }
#endif

    int ret = rtc_set_time(&rtc, &rtcTime);
    if (ret < 0) {
        LOG_ERR("Failed to set RTC time: %d", ret);
        return ret;
    }
    return 0;
}

int CRtc::SetTime(tm& time) {
    return SetTime(*reinterpret_cast<rtc_time*>(&time));
}

int CRtc::SetUnixTime(time_t unixTimestamp) {
    tm* tmTime = gmtime(&unixTimestamp);
    if (tmTime == nullptr) {
        LOG_ERR("Failed to convert UNIX time to tm structure");
        return -1;
    }

    return SetTime(*reinterpret_cast<rtc_time*>(tmTime));
}


