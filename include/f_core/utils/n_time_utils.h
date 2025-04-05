#ifndef N_TIME_UTILS_H
#define N_TIME_UTILS_H

#include <zephyr/drivers/rtc.h>
#include <zephyr/net/sntp.h>
#include <zephyr/sys/timeutil.h>

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

    /**
     * Initialize a time synchronization context for managing time drift between modules
     * 
     * @param[in] rtc The RTC device to use for local time
     * @param[in] syncIntervalSeconds How often to check and compensate for drift (in seconds)
     * @param[in] syncThresholdMs Threshold in milliseconds that triggers a resynchronization
     * @param[in] serverAddress The address of the SNTP server for resynchronization
     * @param[in] maxRetries Max retries when connecting to SNTP server
     * 
     * @return 0 on success, negative error code on failure
     */
    int InitTimeDriftCompensation(CRtc& rtc, uint32_t syncIntervalSeconds, uint32_t syncThresholdMs, 
                                  const char* serverAddress, int maxRetries);

    /**
     * Update time drift compensation state with current time values
     * Should be called periodically to maintain synchronization
     * 
     * @return 0 if no sync needed, 1 if resynchronization was performed, negative on error
     */
    int UpdateTimeDriftCompensation();

    /**
     * Stop time drift compensation
     * 
     * @return 0 on success
     */
    int StopTimeDriftCompensation();

#endif
}

#endif //N_TIME_UTILS_H
