/*
 * Copyright (c) 2023 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <launch_core_classic/os/time.h>
#include <zephyr/kernel.h>

static uint32_t time_of_day_ms = 0;
static uint32_t uptime_during_init_ms = 0;

void l_init_time(uint32_t time_of_day_ms) {
    time_of_day_ms = time_of_day_ms;
    uptime_during_init_ms = k_uptime_get_32();
}

uint32_t l_get_time_of_day_ms(uint32_t current_uptime_ms) {
    if (uptime_during_init_ms == 0) {
        return 0;
    }

    return time_of_day_ms + (current_uptime_ms - uptime_during_init_ms);
}
