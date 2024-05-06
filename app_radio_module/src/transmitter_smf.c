#include "transmitter_smf.h"

#include <zephyr/smf.h>

// State Machine
static void ground_state_run(void *);

static void flight_state_run(void *);

#ifdef CONFIG_DEBUG

static void debug_state_run(void *);

#endif

const struct smf_state transmitter_states[] = {
        [GROUND_STATE] = SMF_CREATE_STATE(NULL, ground_state_run, NULL),
        [FLIGHT_STATE] = SMF_CREATE_STATE(NULL, flight_state_run, NULL),
#ifdef CONFIG_DEBUG
        [DEBUG_STATE] = SMF_CREATE_STATE(NULL, debug_state_run, NULL),
#endif
};

/**
 * Listen to port 9999 and broadcast GNSS
 */
static void ground_state_run(void *) {

}

static void flight_state_run(void *) {

}

#ifdef CONFIG_DEBUG

static void debug_state_run(void *) {

}

#endif