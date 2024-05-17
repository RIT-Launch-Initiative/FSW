/*
 * Copyright (c) 2023 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "power_module.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/smf.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_POWER_MODULE_LOG_LEVEL);

#define DEFINE_STATE_FUNCTIONS(state_name)                                                                             \
    static void state_name##_state_entry(void *);                                                                      \
    static void state_name##_state_run(void *);                                                                        \
    static void state_name##_state_exit(void *);

struct s_object {
    struct smf_ctx ctx;
    bool enable_logging;
} state_obj;

DEFINE_STATE_FUNCTIONS(ground);
DEFINE_STATE_FUNCTIONS(flight);

static const struct smf_state transmitter_states[] = {
    [GROUND_STATE] = SMF_CREATE_STATE(ground_state_entry, ground_state_run, NULL),
    [FLIGHT_STATE] = SMF_CREATE_STATE(flight_state_entry, flight_state_run, NULL),
};

static void ground_state_entry(void *) {
    LOG_INF("Entered ground state");
    state_obj.enable_logging = false;
}

static void ground_state_run(void *) {
    while (true) {
    }
}
static void flight_state_entry(void *) {
    LOG_INF("Entered flight state");
    state_obj.enable_logging = true;
}

static void flight_state_run(void *) {
    while (true) {
    }
}

int main(void) {

    while (true) {
    }
    return 0;
}
