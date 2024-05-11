#ifndef RADIO_MODULE_TRANSMITTER_SMF_H
#define RADIO_MODULE_TRANSMITTER_SMF_H

#include <zephyr/smf.h>

typedef enum {
    GROUND_STATE = 0,
    FLIGHT_STATE,
} TRANSMITTER_STATES;

/**
 * Initializes the state machine
 */
void init_state_machine();

/**
 * Runs through state machine logic
 */
void run_state_machine();

#endif //RADIO_MODULE_TRANSMITTER_SMF_H
