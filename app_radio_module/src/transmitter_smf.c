#include "transmitter_smf.h"
#include "radio_module_functionality.h"

#include <launch_core/net/udp.h>
#include <launch_core/backplane_defs.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(transmitter_smf);

// External variables
extern struct k_timer gnss_tx_timer;

// State Machine Variables
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

struct s_object {
    struct smf_ctx ctx;
    uint32_t altitude;
    bool boost_detected;
} state_obj;

static void boost_detector(struct k_timer *timer_id) {
    static const uint32_t BOOST_THRESHOLD = 500; // 500 ft.
    static uint32_t prev_altitude = 0;

    if ((state_obj.altitude - prev_altitude) < BOOST_THRESHOLD) {
        prev_altitude = state_obj.altitude;
        return;
    }

    state_obj.boost_detected = true;
    k_timer_stop(timer_id);
}

K_TIMER_DEFINE(boost_detect_timer, boost_detector, NULL);

static void ground_state_entry(void *) {
    LOG_INF("Entered ground state");
    k_timer_start(&gnss_tx_timer, K_MSEC(5000), K_MSEC(5000));
    state_obj.boost_detected = false;
}

static void ground_state_run(void *) {
    const int sock = l_init_udp_socket(RADIO_MODULE_IP_ADDR, LAUNCH_EVENT_NOTIFICATION_PORT);
    uint8_t notif = 0;

    while (true) {
        // Monitor port 9999
        if (l_receive_udp(sock, &notif, 1) == 1) {
            // If port 9999 boost detected, go to flight state
            LOG_INF("Received notification: %d", notif);
            state_obj.boost_detected = notif == L_BOOST_DETECTED;
        }

        // If GNSS altitude changes, notify everyone and go to flight state
        // Safe to re-broadcast if received from event port
        if (state_obj.boost_detected) {
            notif = L_BOOST_DETECTED;
            smf_set_state(SMF_CTX(&state_obj), &transmitter_states[FLIGHT_STATE]);
            l_send_udp_broadcast(sock, &notif, 1, LAUNCH_EVENT_NOTIFICATION_PORT);
        }
    }
}

static void ground_state_exit(void *) {
    return;
}

static void flight_state_entry(void *) {
    LOG_INF("Entered flight state");
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

void init_state_machine() {
    smf_set_initial(SMF_CTX(&state_obj), &transmitter_states[GROUND_STATE]);
}

void run_state_machine() {
    static int ret = 0;
    if (ret == 0) {
        ret = smf_run_state(SMF_CTX(&state_obj));
    }
}