#ifndef N_TIME_UTILS_H
#define N_TIME_UTILS_H

#include <zephyr/drivers/rtc.h>
#include <zephyr/net/sntp.h>

class CRtc;
namespace NTimeUtils {
#ifdef CONFIG_SNTP
	/**
	 * Synchronize the time with an SNTP server
	 *
	 * @param[in] rtc The RTC device to set the time on
	 * @param[in] serverAddress The address of the SNTP server
	 * @param[in] maxRetries The maximum number of retries before giving up
	 *
	 * @return 0 on success, -1 on failure
	 */
    int SntpSynchronize(CRtc& rtc, const char *serverAddress, const int maxRetries);
#endif
}

#endif //N_TIME_UTILS_H
