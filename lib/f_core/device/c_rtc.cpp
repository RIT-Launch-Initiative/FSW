#include "f_core/device/c_rtc.h"

#include <time.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/rtc.h>

LOG_MODULE_REGISTER(CRtc);

CRtc::CRtc(const device& dev) : rtc(dev) {}

int CRtc::GetTime(rtc_time& time) {
    int ret = rtc_get_time(&rtc, &time);
    if (ret < 0) {
        LOG_ERR("Failed to get RTC time: %d", ret);
        return ret;
    }
    return 0;
}

int CRtc::GetTime(tm& time) {
    return GetTime(reinterpret_cast<rtc_time&>(time));
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


