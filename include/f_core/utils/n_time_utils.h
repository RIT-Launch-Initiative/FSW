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
	 * @param[in] retryDelay The delay between retries (default is 100ms)
	 *
	 * @return 0 on success, -1 on failure
	 */
    int SntpSynchronize(CRtc& rtc, const char *serverAddress, int maxRetries, k_timeout_t retryDelay = K_MSEC(100));

    /**
     * Set up a callback to synchronize the RTC with an SNTP server at regular intervals
     * NOTE: Be smart about the max retries. 100ms delay per
     *
     * @param rtc RTC to be set
     * @param interval Interval in milliseconds for the callback to be called
     * @param maxRetries Max retries in a callback
     * @param serverAddress Address of the SNTP server
     * @param retryDelay Delay between retries in milliseconds
     */
     // void SetupSntpSynchronizationCallback(CRtc& rtc, const int interval, const char* serverAddress, const int maxRetries, const k_timeout_t retryDelay = K_MSEC(100));



}

#endif //N_TIME_UTILS_H
