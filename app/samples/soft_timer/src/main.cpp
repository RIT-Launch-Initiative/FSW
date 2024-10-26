/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/os/n_rtos.h>
#include <f_core/utils/c_soft_timer.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

static void timerExpirationFn(k_timer* timer) {
    LOG_INF("Timer expired");
}

static void timerStopFn(k_timer* timer) {
    LOG_INF("Timer stopped");
}

int main() {
    CSoftTimer timer(timerExpirationFn, timerStopFn);
    timer.StartTimer(100);

    int interruptCount = 0;

    while (interruptCount < 5) {
        if (timer.IsExpired()) {
            interruptCount++;
        }

        k_sleep(K_MSEC(25));
        LOG_INF("Remaining time: %d ms %d ticks", timer.GetRemainingMillis(), timer.GetRemainingTicks());
    }

    LOG_INF("Waiting for timer to expire");
    timer.BlockUntilExpired();

    LOG_INF("Stopping timer");
    timer.StopTimer();

    return 0;
}
