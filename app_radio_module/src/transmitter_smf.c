#include "transmitter_smf.h"

#include <zephyr/smf.h>

// State Machine
#define DEFINE_STATE_FUNCTIONS(state_name)          \
static void state_name##_state_entry(void *);       \
static void state_name##_state_run(void *);         \
static void state_name##_state_exit(void *);

DEFINE_STATE_FUNCTIONS(ground);
DEFINE_STATE_FUNCTIONS(flight);

static const struct smf_state transmitter_states[] = {
        [GROUND_STATE] = SMF_CREATE_STATE(ground_state_entry, ground_state_run, ground_state_exit),
        [FLIGHT_STATE] = SMF_CREATE_STATE(flight_state_entry, flight_state_run, flight_state_exit),
};

static void ground_state_entry(void *) {
    // Configure GNSS timer for 5 seconds
}

static void ground_state_run(void *) {
    while (true) {
        // Monitor port 9999

        // If port 9999 boost detected, go to flight state

        // If GNSS altitude changes, notify everyone and go to flight state
    }
}

static void ground_state_exit(void *) {
    return;
}

static void flight_state_entry(void *) {
    // Start timer
}

static void flight_state_run(void *) {
    while (true) {
        // Listen to all ports

        // If notified of landing, go back to ground state.

        // If timer expires, dump data over LoRa
    }
}

static void flight_state_exit(void *) {
    // Stop timer
}
