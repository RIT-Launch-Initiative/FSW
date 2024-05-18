/*
 * Copyright (c) 2023 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "power_module.h"

#include <launch_core/utils/event_monitor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/smf.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_POWER_MODULE_LOG_LEVEL);

#define DEFINE_STATE_FUNCTIONS(state_name)                                                                             \
    static void state_name##_state_entry(void *);                                                                      \
    static void state_name##_state_run(void *);                                                                        \
    static void state_name##_state_exit(void *) {}

struct s_object {
    struct smf_ctx ctx;
} state_obj;

bool logging_enabled; // Keep separate from s_object to eliminate extra #includes in other src fiels

DEFINE_STATE_FUNCTIONS(ground);
DEFINE_STATE_FUNCTIONS(flight);

static const struct smf_state transmitter_states[] = {
    [GROUND_STATE] = SMF_CREATE_STATE(ground_state_entry, ground_state_run, ground_state_exit),
    [FLIGHT_STATE] = SMF_CREATE_STATE(flight_state_entry, flight_state_run, flight_state_exit),
};

static void ground_state_entry(void *) {
    LOG_INF("Entered ground state");
    logging_enabled = false;
}

static void ground_state_run(void *) {
    while (true) {
        if (l_get_event_udp() == L_BOOST_DETECTED) {
            smf_set_state(SMF_CTX(&state_obj), &transmitter_states[FLIGHT_STATE]);
            return;
        }
    }
}
static void flight_state_entry(void *) {
    LOG_INF("Entered flight state");
    logging_enabled = true;
}

static void flight_state_run(void *) {
    while (true) {
        if (l_get_event_udp() == L_LANDING_DETECTED) {
            smf_set_state(SMF_CTX(&state_obj), &transmitter_states[GROUND_STATE]);
            return;
        }
    }
}

static void init() {
    init_networking();

    smf_set_initial(SMF_CTX(&state_obj), &transmitter_states[GROUND_STATE]);
    l_init_event_monitor(POWER_MODULE_IP_ADDR);
}

int main(void) {
    init();

    while (true) {
        static int ret = 0;
        if (ret == 0) {
            ret = smf_run_state(SMF_CTX(&state_obj));
            if (ret < 0) {
                LOG_ERR("Failed to run state machine: %d", ret);
            }
        }
    }

    return 0;
}
