#pragma once

#include <time.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>

class CRtc {
public:
    explicit CRtc(const device& dev);

    int GetTime(rtc_time& time);

    int GetTime(tm& time);

    int GetMillisTime(uint32_t& millis);

    int GetMillisTime(uint64_t& millis);

    int GetUnixTime(time_t &unixTimestamp);

    int GetUnixTime(uint32_t &unixTimestamp);

    int SetTime(rtc_time& rtcTime);

    int SetTime(tm& time);

    int SetUnixTime(time_t unixTimestamp);

private:
    static constexpr uint32_t RTC_UNIX_SECONDS_DIFFERENCE = 2208988800; // Seconds between 1900 and 1970

    const device& rtc;

};

#endif //C_RTC_H
