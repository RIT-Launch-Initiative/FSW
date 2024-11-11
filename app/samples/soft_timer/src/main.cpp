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


static void timerExpirationFn(k_timer*) {
    LOG_INF("Timer expired");
    interruptCount++;
}

static void timerStopFn(k_timer*) {
    LOG_INF("Timer stopped");
}

int main() {
    CSoftTimer timer(timerExpirationFn, timerStopFn);

    timer.StartTimer(100);
    LOG_INF("Starting 100ms timer");

    while (interruptCount < 5) {
        k_msleep(80);
        LOG_INF("\tRemaining time: %d ms %d ticks", timer.GetRemainingMillis(), timer.GetRemainingTicks());
    }

    LOG_INF("Setting 100ms timer to 1s");

    timer.StartTimer(1000);
    LOG_INF("Blocking wait for 1s timer");

    timer.BlockUntilExpired();

    LOG_INF("Expired! Tests complete. Stopping timer.");
    // Intentionally don't call StopTimer to test destructor

    return 0;
}
