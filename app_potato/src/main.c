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
volatile uint8_t event_byte = 0;
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

static inline void signal_state_transition() { state_transition = true; }
K_TIMER_DEFINE(state_transition_timer, signal_state_transition, NULL);

static void update_state_transition_timer(uint32_t duration_seconds) {
    k_timer_start(&state_transition_timer, K_SECONDS(duration_seconds), K_SECONDS(duration_seconds));
    state_transition = false;
}

static void pad_state_entry(void *) {
    LOG_INF("Entering pad state");
    start_boost_detect();
}

static void pad_state_run(void *) {
    while (true) {
        bool received_boost_notif = (L_BOOST_DETECTED == event_byte);

        if (boost_detected || received_boost_notif) {
            smf_set_state(SMF_CTX(&state_obj), &states[BOOST_STATE]);
            return;
        }
        k_msleep(1);
    }
}
#if 0
#define BOOST_TRANS_TIME         3
#define COAST_TRANS_TIME         23
#define APOGEE_TRANS_TIME        152
#define MAIN_TRANS_TIME          46
#define LANDING_STATE_EXTRA_TIME 60
#else
#define BOOST_TRANS_TIME         1
#define COAST_TRANS_TIME         5
#define APOGEE_TRANS_TIME        5
#define MAIN_TRANS_TIME          5
#define LANDING_STATE_EXTRA_TIME 5

#endif

static void boost_state_entry(void *) {
    LOG_INF("Entering boost state");
    // TODO: Should get rid of magic numbers. Maybe create a directory of flight configurations with constants?
    update_state_transition_timer(BOOST_TRANS_TIME);
    stop_boost_detect();
    logging_enabled = true;
}

static void boost_state_run(void *) {
    while (!state_transition) {
        k_msleep(10);
    }
    smf_set_state(SMF_CTX(&state_obj), &states[COAST_STATE]);
}

static void coast_state_entry(void *) {
    LOG_INF("Entering coast state");
    configure_telemetry_rate(1337); // TODO
    update_state_transition_timer(COAST_TRANS_TIME);
}

static void coast_state_run(void *) {
    while (!state_transition) {
        k_sleep(K_MSEC(10));
    }
    smf_set_state(SMF_CTX(&state_obj), &states[APOGEE_STATE]);
}

static void apogee_state_entry(void *) {
    LOG_INF("Entering apogee state");
    configure_telemetry_rate(420); // TODO
    update_state_transition_timer(APOGEE_TRANS_TIME);
}

static void apogee_state_run(void *) {
    while (!state_transition) {
        k_msleep(10);
    }
    smf_set_state(SMF_CTX(&state_obj), &states[MAIN_STATE]);
}

static void main_state_entry(void *) {
    LOG_INF("Entering main chute state");
    configure_telemetry_rate(21); // TODO: Update with proper telemetry rates
    update_state_transition_timer(MAIN_TRANS_TIME);
}

static void main_state_run(void *) {
    while (!state_transition) {
        k_msleep(10);
    }

    smf_set_state(SMF_CTX(&state_obj), &states[LANDING_STATE]);
}

static void landing_state_entry(void *) { LOG_INF("Entering landing state"); }

static void landing_state_run(void *) {
    // Sleep for an extra minute and ensure we get all our data!
    k_sleep(K_SECONDS(LANDING_STATE_EXTRA_TIME));
    logging_enabled = false;
    LOG_INF("Stopped Logging");
    k_sleep(K_FOREVER);
}

int main() {
    boot_count = l_fs_boot_count_check();
    // smf_set_initial(SMF_CTX(&state_obj), &states[PAD_STATE]);
    //
    // while (true) {
    //     k_msleep(100);
    //     static int ret = 0;
    //     if (ret == 0) {
    //         ret = smf_run_state(SMF_CTX(&state_obj));
    //         if (ret < 0) {
    //             LOG_ERR("Failed to run state machine: %d", ret);
    //         }
    //     }
    // }
    init_modbus_server();

    return 0;
}
