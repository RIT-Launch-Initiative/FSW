#include "f_core/device/c_rtc.h"
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

int CRtc::GetUnixTime(uint32_t unixTimestamp) {

    return 0;
}

int CRtc::SetTime(rtc_time& time) {
    int ret = rtc_set_time(&rtc, &time);
    if (ret < 0) {
        LOG_ERR("Failed to set RTC time: %d", ret);
        return ret;
    }
    return 0;
}

int CRtc::SetTime(tm& time) {
    return SetTime(reinterpret_cast<rtc_time&>(time));
}

int CRtc::SetUnixTime(uint32_t unixTimestamp) {
    return 0;
}


