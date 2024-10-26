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

volatile int interruptCount = 0;


static void timerExpirationFn(k_timer* timer) {
    LOG_INF("Timer expired");
    interruptCount++;
}

static void timerStopFn(k_timer* timer) {
    LOG_INF("Timer stopped");
}

int main() {
    CSoftTimer timer(timerExpirationFn, timerStopFn);

    timer.StartTimer(100);
    LOG_INF("Starting timer");

    while (interruptCount < 5) {
        k_msleep(80);
        LOG_INF("\tRemaining time: %d ms %d ticks", timer.GetRemainingMillis(), timer.GetRemainingTicks());
    }

    LOG_INF("Blocking wait for timer to expire");
    timer.BlockUntilExpired();

    LOG_INF("Expired! Tests complete. Stopping timer.");
    timer.StopTimer();

    return 0;
}
