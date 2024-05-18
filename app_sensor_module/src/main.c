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

LOG_MODULE_REGISTER(main, CONFIG_APP_SENSOR_MODULE_LOG_LEVEL);

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


struct s_object {
    struct smf_ctx ctx;
} state_obj;

static void pad_state_entry(void*) {
    LOG_INF("Entering pad state");
}

static void pad_state_run(void*) {
    while (true) {

    }
}

static void pre_main_state_entry(void*) {
    LOG_INF("Entering pre_main state");
}

static void pre_main_state_run(void*) {
    while (true) {

    }
}

static void post_main_state_entry(void*) {
    LOG_INF("Entering post_main state");
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

    smf_set_initial(SMF_CTX(&state_obj), NULL);
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
