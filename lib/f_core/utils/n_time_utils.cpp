/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "f_core/utils/n_time_utils.h"
#include <f_core/device/c_rtc.h>
#include <f_core/utils/c_soft_timer.h>
#include <zephyr/sys/timeutil.h>

LOG_MODULE_REGISTER(NTimeUtils);

#ifdef CONFIG_SNTP
int NTimeUtils::SntpSynchronize(CRtc& rtc, const char* serverAddress, const int maxRetries, const k_timeout_t retryDelay) {
    int retryCount = 0;
    sntp_time ts{0};

    // Note this is a 100ms timeout. Zephyr does a poor job of documenting this.
    LOG_INF("Synchronizing time using NTP with server %s", serverAddress);
    while (sntp_simple(serverAddress, 10, &ts) && retryCount < maxRetries) {
        k_sleep(retryDelay);
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

// void NTimeUtils::SetupSntpSynchronizationCallback(CRtc& rtc, const int interval, const char* serverAddress, const int maxRetries, const k_timeout_t retryDelay) {
//     // TODO: Timeout in sntp_simple gets things stuck so don't do use this function (yet)
//     // Maybe we just set up another tenant or create a new thread that we can suspend and resume using the timer
//     LOG_ERR("DO NOT USE. BUGGED");
//     k_oops();
//
//
//     static std::tuple<CRtc&, const char*, int, const k_timeout_t> sntpData{rtc, serverAddress, maxRetries, retryDelay};
//     static auto sntpCallback = [](k_timer* timer) {
//         auto data = static_cast<std::tuple<CRtc&, const char*, int, k_timeout_t>*>(k_timer_user_data_get(timer));
//         CRtc& rtc = std::get<0>(*data); // Note that RTC drivers "should be" thread safe, so KISS and don't do any synchronization here
//         const char* serverAddress = std::get<1>(*data);
//         int maxRetries = std::get<2>(*data);
//         k_timeout_t retryDelay = std::get<3>(*data);
//
//         SntpSynchronize(rtc, serverAddress, maxRetries, retryDelay);
//     };
//
//     static CSoftTimer timer{sntpCallback};
//     if (timer.IsRunning()) {
//         LOG_WRN("SNTP synchronization is already running!");
//         return;
//     }
//
//     timer.SetUserData(&sntpData);
//     timer.StartTimer(interval, 0);
// }

namespace {
    struct TimeDriftCompensationData {
        CRtc* rtc;
        const char* serverAddress;
        int maxRetries;
        uint32_t syncIntervalSeconds;
        uint32_t syncThresholdMs;
        timeutil_sync_state syncState;
        timeutil_sync_config syncConfig;
        bool initialized;
        CSoftTimer* driftCheckTimer;
    };

    TimeDriftCompensationData driftCompData = {
        .rtc = nullptr,
        .serverAddress = nullptr,
        .maxRetries = 0,
        .syncIntervalSeconds = 0,
        .syncThresholdMs = 0,
        .syncState = {},
        .syncConfig = {},
        .initialized = false,
        .driftCheckTimer = nullptr
    };

    auto driftCheckCallback = [](k_timer*) {
        if (NTimeUtils::UpdateTimeDriftCompensation() < 0) {
            LOG_WRN("Failed to update time drift compensation");
        }
    };
}

static uint64_t getUptimeTicks() {
    // Note that under the hood Zephyr uses a uint64_t and then return casts it to int64_t :P
    return static_cast<uint64_t>(k_uptime_ticks());
}

int NTimeUtils::InitTimeDriftCompensation(CRtc& rtc, uint32_t syncIntervalSeconds, uint32_t syncThresholdMs,
                                          const char* serverAddress, int maxRetries) {
    if (driftCompData.initialized) {
        LOG_WRN("Time drift compensation already initialized");
        return -EALREADY;
    }

    if (syncIntervalSeconds == 0 || syncThresholdMs == 0) {
        LOG_ERR("Invalid sync interval or threshold");
        return -EINVAL;
    }

    // configuration params
    driftCompData.rtc = &rtc;
    driftCompData.serverAddress = serverAddress;
    driftCompData.maxRetries = maxRetries;
    driftCompData.syncIntervalSeconds = syncIntervalSeconds;
    driftCompData.syncThresholdMs = syncThresholdMs;
    
    driftCompData.syncConfig.ref_Hz = 1; // Reference time in seconds (SNTP)
    driftCompData.syncConfig.local_Hz = 1000; // Local time in millis
    driftCompData.syncState.cfg = &driftCompData.syncConfig;
    driftCompData.syncState.skew = 1.0f; // Initial skew assuming perfect timing

    // Perform an initial synchronization
    LOG_INF("Initializing time drift compensation");
    if (SntpSynchronize(rtc, serverAddress, maxRetries) != 0) {
        LOG_ERR("Failed initial SNTP synchronization");
        return -EIO;
    }

    // Record initial sync point
    time_t refTime;
    if (rtc.GetUnixTime(refTime) != 0) {
        LOG_ERR("Failed to get reference time from RTC");
        return -EIO;
    }

    // Set base time reference point
    timeutil_sync_instant syncPoint = {
        .ref = static_cast<uint64_t>(refTime),
        .local = getUptimeTicks()
    };
    
    if (timeutil_sync_state_update(&driftCompData.syncState, &syncPoint) < 0) {
        LOG_ERR("Failed to update sync state");
        return -EIO;
    }

    static CSoftTimer driftTimer{driftCheckCallback};
    driftCompData.driftCheckTimer = &driftTimer;
    driftCompData.driftCheckTimer->StartTimer(syncIntervalSeconds * 1000, syncIntervalSeconds * 1000);
    driftCompData.initialized = true;
    LOG_INF("Time drift compensation initialized with %d second check interval", syncIntervalSeconds);
    
    return 0;
}

int NTimeUtils::UpdateTimeDriftCompensation() {
    if (!driftCompData.initialized) {
        LOG_ERR("Time drift compensation not initialized");
        return -EINVAL;
    }

    time_t currentRtcTime;
    if (driftCompData.rtc->GetUnixTime(currentRtcTime) != 0) {
        LOG_ERR("Failed to get current RTC time");
        return -EIO;
    }

    // Calculate expected reference time based on local time and skew
    uint64_t localTimeMs = getUptimeTicks();
    uint64_t expectedRefTime;
    
    int ret = timeutil_sync_ref_from_local(&driftCompData.syncState, localTimeMs, &expectedRefTime);
    if (ret < 0) {
        LOG_ERR("Failed to calculate expected reference time: %d", ret);
        return ret;
    }

    // Compare RTC time with expected time from sync calc
    int64_t timeDiffMs = (currentRtcTime - static_cast<int64_t>(expectedRefTime)) * 1000
    
    LOG_INF("Time drift check: actual=%lld expected=%lld diff=%lld ms",
            (long long)currentRtcTime, (long long)expectedRefTime, (long long)timeDiffMs);
    
    // If drift exceeds threshold, resync
    if (abs(timeDiffMs) > driftCompData.syncThresholdMs) {
        LOG_WRN("Time drift of %lld ms exceeds threshold of %d ms, resynchronizing", 
                (long long)timeDiffMs, driftCompData.syncThresholdMs);
        
        // Rsync over SNTP
        if (SntpSynchronize(*driftCompData.rtc, driftCompData.serverAddress, driftCompData.maxRetries) != 0) {
            LOG_ERR("Failed to resynchronize time");
            return -EIO;
        }
        
        time_t newRtcTime;
        if (driftCompData.rtc->GetUnixTime(newRtcTime) != 0) {
            LOG_ERR("Failed to get updated RTC time after sync");
            return -EIO;
        }
        
        timeutil_sync_instant newSyncPoint = {
            .ref = static_cast<uint64_t>(newRtcTime),
        };
        newSyncPoint.local = getUptimeTicks(); // Weird error when this is brace initialized

        
        if (timeutil_sync_state_update(&driftCompData.syncState, &newSyncPoint) < 0) {
            LOG_ERR("Failed to update sync state after resync");
            return -EIO;
        }
        
        // Calc new skew based on the latest measurements
        float newSkew = timeutil_sync_estimate_skew(&driftCompData.syncState);
        if (newSkew > 0.0f) {
            LOG_INF("Updated clock skew: %f (PPB: %d)", newSkew, timeutil_sync_skew_to_ppb(newSkew));
            timeutil_sync_state_set_skew(&driftCompData.syncState, newSkew, NULL);
        }
        
        return 1; // Intentionally 1. Signal we resynced
    }
    
    // Record current time point for future skew calcs, but dont update on every check
    timeutil_sync_instant currentSyncPoint = {
        .ref = static_cast<uint64_t>(currentRtcTime),
        .local = localTimeMs
    };
    
    if (timeutil_sync_state_update(&driftCompData.syncState, &currentSyncPoint) < 0) {
        LOG_ERR("Failed to update latest sync point");
        return -EIO;
    }
    
    return 0;
}

int NTimeUtils::StopTimeDriftCompensation() {
    if (!driftCompData.initialized) {
        LOG_WRN("Time drift compensation not initialized");
        return -EINVAL;
    }
    
    if (driftCompData.driftCheckTimer) {
        driftCompData.driftCheckTimer->StopTimer();
    }
    
    driftCompData.initialized = false;
    LOG_INF("Time drift compensation stopped");
    
    return 0;
}

#endif