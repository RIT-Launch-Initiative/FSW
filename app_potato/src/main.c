// Self Include
#include "potato.h"

// Launch Includes
#include <launch_core/backplane_defs.h>
#include <launch_core/os/fs.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/smf.h>

LOG_MODULE_REGISTER(app_potato);

// Queues
static K_QUEUE_DEFINE(slip_tx_queue);

// Extern Variables
extern bool boost_detected;

// Global Variables
bool logging_enabled = false;
bool state_transition = true;
uint32_t boot_count = -1;

// State Machine
#define DEFINE_STATE_FUNCTIONS(state_name)                                                                             \
    static void state_name##_state_entry(void *);                                                                      \
    static void state_name##_state_run(void *);                                                                        \
    static void state_name##_state_exit(void *) {}

DEFINE_STATE_FUNCTIONS(pad);
DEFINE_STATE_FUNCTIONS(boost);
DEFINE_STATE_FUNCTIONS(coast);
DEFINE_STATE_FUNCTIONS(apogee);
DEFINE_STATE_FUNCTIONS(main);
DEFINE_STATE_FUNCTIONS(landing);

static const struct smf_state states[] = {
    [PAD_STATE] = SMF_CREATE_STATE(pad_state_entry, pad_state_run, pad_state_exit),
    [BOOST_STATE] = SMF_CREATE_STATE(boost_state_entry, boost_state_run, boost_state_exit),
    [COAST_STATE] = SMF_CREATE_STATE(coast_state_entry, coast_state_run, coast_state_exit),
    [APOGEE_STATE] = SMF_CREATE_STATE(apogee_state_entry, apogee_state_run, apogee_state_exit),
    [MAIN_STATE] = SMF_CREATE_STATE(main_state_entry, main_state_run, main_state_exit),
    [LANDING_STATE] = SMF_CREATE_STATE(landing_state_entry, landing_state_run, landing_state_exit),
};

struct s_object {
    struct smf_ctx ctx;
} state_obj;

static void signal_state_transition() { state_transition = true; }
K_TIMER_DEFINE(state_transition_timer, signal_state_transition, NULL);

static void update_state_transition_timer(uint32_t duration_seconds) {
    k_timer_start(&state_transition_timer, K_SECONDS(duration_seconds), K_SECONDS(duration_seconds));
    state_transition = false;
}

static void pad_state_entry(void*) {
    LOG_INF("Entering pad state");
}

static void pad_state_run(void*) {
    while (true) {
        bool received_boost_notif = (L_BOOST_DETECTED == get_event_from_serial());

        // If GNSS altitude changes, notify everyone and go to flight state
        if (boost_detected || received_boost_notif) {
            smf_set_state(SMF_CTX(&state_obj), &states[BOOST_STATE]);
            return;
        }
    }
}

static void boost_state_entry(void*) {
    LOG_INF("Entering boost state");
    // TODO: Should get rid of magic numbers. Maybe create a directory of flight configurations with constants?
    update_state_transition_timer(3);
}

static void boost_state_run(void*) {

}

static void coast_state_entry(void*) {
    LOG_INF("Entering coast state");
    configure_telemetry_rate(1337); // TODO
    update_state_transition_timer(23);
}

static void coast_state_run(void*) {

}

static void apogee_state_entry(void*) {
    LOG_INF("Entering apogee state");
    configure_telemetry_rate(420); // TODO
    update_state_transition_timer(152);

}

static void apogee_state_run(void*) {

}

static void main_state_entry(void*) {
    LOG_INF("Entering main chute state");
    configure_telemetry_rate(21); // TODO
    update_state_transition_timer(46);
}

static void main_state_run(void*) {

}

static void landing_state_entry(void*) {
    LOG_INF("Entering landing state");

}

static void landing_state_run(void*) {

}

int main() {
    boot_count = l_fs_boot_count_check();

    while (true) {
        k_msleep(100);
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
