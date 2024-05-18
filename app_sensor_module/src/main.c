// Self Include
#include "sensor_module.h"

// Launch Includes
#include <launch_core/backplane_defs.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/utils/event_monitor.h>

// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_SENSOR_MODULE_LOG_LEVEL);

#define PRE_MAIN_FLIGHT_DURATION K_SECONDS(178)
#define POST_MAIN_FLIGHT_DURATION K_SECONDS(46)

#define DEFINE_STATE_FUNCTIONS(state_name)                                                                             \
    static void state_name##_state_entry(void *);                                                                      \
    static void state_name##_state_run(void *);                                                                        \
    static void state_name##_state_exit(void *) {}

DEFINE_STATE_FUNCTIONS(pad);
DEFINE_STATE_FUNCTIONS(pre_main);
DEFINE_STATE_FUNCTIONS(post_main);
DEFINE_STATE_FUNCTIONS(landing);

extern bool boost_detected;
static bool in_flight_transition_event = false;

struct s_object {
    struct smf_ctx ctx;
} state_obj;

static const struct smf_state states[] = {
    [PAD_STATE] = SMF_CREATE_STATE(pad_state_entry, pad_state_run, pad_state_exit),
    [PRE_MAIN_STATE] = SMF_CREATE_STATE(pre_main_state_entry, pre_main_state_run, pre_main_state_exit),
    [POST_MAIN_STATE] = SMF_CREATE_STATE(post_main_state_entry, post_main_state_run, post_main_state_exit),
    [LANDING_STATE] = SMF_CREATE_STATE(landing_state_entry, landing_state_run, landing_state_exit),
};


static void state_transition_timer_cb(struct k_timer*) {
    in_flight_transition_event = true;
}

K_TIMER_DEFINE(state_transition_timer, state_transition_timer_cb, NULL);


static void pad_state_entry(void*) {
    LOG_INF("Entering pad state");
    start_boost_detect();
}

static void pad_state_run(void*) {
    while (true) {
        // If GNSS altitude changes, notify everyone and go to flight state
        if (boost_detected) {
            smf_set_state(SMF_CTX(&state_obj), &states[PRE_MAIN_STATE]);
            l_post_event_udp(L_BOOST_DETECTED);
            return;
        }

        // Check port 9999 for notifications. If we get one, go to flight state on next iter
        boost_detected = l_get_event_udp() == L_BOOST_DETECTED;
    }
}

static void pre_main_state_entry(void*) {
    LOG_INF("Entering pre_main state");
    stop_boost_detect();
    k_timer_start(&state_transition_timer, PRE_MAIN_FLIGHT_DURATION, PRE_MAIN_FLIGHT_DURATION);

    in_flight_transition_event = false;
}

static void pre_main_state_run(void*) {
    while (true) {
        if (in_flight_transition_event) {
            l_post_event_udp(L_MAIN_DEPLOYED);
            smf_set_state(SMF_CTX(&state_obj), &states[POST_MAIN_STATE]);

            return;
        }
    }
}

static void post_main_state_entry(void*) {
    LOG_INF("Entering post_main state");
    k_timer_start(&state_transition_timer, POST_MAIN_FLIGHT_DURATION, POST_MAIN_FLIGHT_DURATION);

    in_flight_transition_event = false;
}

static void post_main_state_run(void*) {
    while (true) {
        if (in_flight_transition_event) {
            l_post_event_udp(L_LANDING_DETECTED);
            smf_set_state(SMF_CTX(&state_obj), &states[LANDING_STATE]);

            return;
        }
    }
}


static void landing_state_entry(void*) {
    LOG_INF("Entering landing state");
}

static void landing_state_run(void*) {
    while (true) {
        k_sleep(K_FOREVER);
    }
}

static void init() {
    init_networking();

    smf_set_initial(SMF_CTX(&state_obj), &states[PAD_STATE]);
    l_init_event_monitor(SENSOR_MODULE_IP_ADDR);
}

static void run_state_machine() {
    static int ret = 0;
    if (ret == 0) {
        ret = smf_run_state(SMF_CTX(&state_obj));
        if (ret < 0) {
            LOG_ERR("Failed to run state machine: %d", ret);
        }
    }
}

int main() {
    static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
    static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

    init();

    while (true) {
        gpio_pin_toggle_dt(&led0);
        gpio_pin_toggle_dt(&led1);
        run_state_machine();
    }

    return 0;
}
