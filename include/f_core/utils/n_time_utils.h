#ifndef N_TIME_UTILS_H
#define N_TIME_UTILS_H


namespace NTimeUtils {
#ifdef CONFIG_SNTP
#include <string>
#include <zephyr/drivers/rtc.h>
#include <zephyr/net/sntp.h>

	/**
	 * Convert an SNTP time to an RTC time
	 *
	 * @param sntp[in] SNTP time to be converted
	 * @param rtc[out] RTC time to store the result to
	 */
	void SntpToRtcTime(const sntp_time &sntp, rtc_time &rtc);

	/**
	 * Synchronize the time with an SNTP server
	 *
	 * @param[in] rtc The RTC device to set the time on
	 * @param[in] serverAddress The address of the SNTP server
	 * @param[in] maxRetries The maximum number of retries before giving up
	 *
	 * @return 0 on success, -1 on failure
	 */
    int SntpSynchronize(const device& rtc, const char *serverAddress, const int maxRetries);
#endif
}

#endif //N_TIME_UTILS_H
