#ifndef C_RTC_H
#define C_RTC_H

#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>

class CRtc {
public:
    CRtc(const device& dev);

    int GetTime(rtc_time& time);

    int GetTime(tm& time);

    int GetUnixTime(uint32_t &unixTimestamp);

    int SetTime(rtc_time& time);

    int SetTime(tm& time);

    int SetUnixTime(uint32_t unixTimestamp);

private:
    const device& rtc;

};

#endif //C_RTC_H
