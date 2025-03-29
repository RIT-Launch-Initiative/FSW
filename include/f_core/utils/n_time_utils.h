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

    /**
     * Set up a callback to synchronize the RTC with an SNTP server at regular intervals
     * NOTE: Be smart about the interval and max retries, because it'll determine how long your callback executes for
     *
     * @param rtc RTC to be set
     * @param serverAddress Address of the SNTP server
     * @param maxRetries Max retries in a callback
     * @param interval Interval in milliseconds for the callback to be called
     */
    void NTimeUtils::SetupSntpSynchronizationCallback(CRtc& rtc, const char* serverAddress, const int maxRetries, int interval);

#endif
}

#endif //N_TIME_UTILS_H
