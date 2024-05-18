// Self Include
#include "sensor_module.h"

// Launch Includes
#include <launch_core/dev/dev_common.h>

// Zephyr Includes
#include <launch_core/utils/event_monitor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/parser_state.h>

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

static const struct smf_state states[] = {
    [PAD_STATE] = SMF_CREATE_STATE(pad_state_entry, pad_state_run, pad_state_exit),
    [PRE_MAIN_STATE] = SMF_CREATE_STATE(pre_main_state_entry, pre_main_state_run, pre_main_state_exit),
    [POST_MAIN_STATE] = SMF_CREATE_STATE(post_main_state_entry, post_main_state_run, post_main_state_exit),
    [LANDING_STATE] = SMF_CREATE_STATE(landing_state_entry, landing_state_run, landing_state_exit),
};

static void state_transition_timer_cb(struct k_timer*) {
    // TODO: Implement state transition logic
}

K_TIMER_DEFINE(state_transition_timer, state_transition_timer_cb, NULL);

struct s_object {
    struct smf_ctx ctx;
} state_obj;

static void pad_state_entry(void*) {
    LOG_INF("Entering pad state");
    start_boost_detect();
}

static void pad_state_run(void*) {
    while (true) {

    }
}

static void pre_main_state_entry(void*) {
    LOG_INF("Entering pre_main state");
    stop_boost_detect();
    k_timer_start(&state_transition_timer, PRE_MAIN_FLIGHT_DURATION, PRE_MAIN_FLIGHT_DURATION);
}

static void pre_main_state_run(void*) {
    while (true) {

    }
}

static void post_main_state_entry(void*) {
    LOG_INF("Entering post_main state");
    k_timer_start(&state_transition_timer, POST_MAIN_FLIGHT_DURATION, POST_MAIN_FLIGHT_DURATION);

}

static void post_main_state_run(void*) {
    while (true) {

    }
}


static void landing_state_entry(void*) {
    LOG_INF("Entering landing state");
}

static void landing_state_run(void*) {
    while (true) {

    }
}

static void init() {
    init_networking();

    smf_set_initial(SMF_CTX(&state_obj), &states[PAD_STATE]);
    l_init_event_monitor(SENSOR_MODULE_IP_ADDR);
}

int main() {
    static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
    static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

    init();

    while (true) {
        gpio_pin_toggle_dt(&led0);
        gpio_pin_toggle_dt(&led1);
        k_msleep(100);
    }

    return 0;
}
