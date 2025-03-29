/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "f_core/utils/n_time_utils.h"
#include <time.h>
#include <f_core/device/c_rtc.h>

LOG_MODULE_REGISTER(NTimeUtils);

#ifdef CONFIG_SNTP
int NTimeUtils::SntpSynchronize(CRtc& rtc, const char* serverAddress, const int maxRetries) {
    int retryCount = 0;
    sntp_time ts{0};

    // Note this is a 100ms timeout. Zephyr does a poor job of documenting this.
    LOG_INF("Synchronizing time using NTP with server %s", serverAddress);
    while (sntp_simple(serverAddress, 1000, &ts) && retryCount < maxRetries) {
        k_sleep(K_SECONDS(1));
        retryCount++;
        LOG_ERR("Failed to synchronize time. Retrying (%d)", retryCount);
    }

    if (retryCount >= maxRetries) {
        LOG_ERR("Failed to synchronize time with server %s", serverAddress);
        return -1;
    }

    LOG_INF("SNTP synchronized with server %s", serverAddress);
    rtc.SetUnixTime(ts.seconds);

    return 0;
}
#endif