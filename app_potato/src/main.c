// Self Include
#include "potato.h"

// Launch Includes
#include <launch_core/os/fs.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/smf.h>

LOG_MODULE_REGISTER(app_potato);

// Queues
static K_QUEUE_DEFINE(slip_tx_queue);

// Global Variables
bool logging_enabled = false;
uint32_t boot_count = -1;

// State Machine
#define DEFINE_STATE_FUNCTIONS(state_name)                                                                             \
    static void state_name##_state_entry(void *);                                                                      \
    static void state_name##_state_run(void *);                                                                        \
    static void state_name##_state_exit(void *) {}

DEFINE_STATE_FUNCTIONS(pad);
DEFINE_STATE_FUNCTIONS(coast);
DEFINE_STATE_FUNCTIONS(apogee);
DEFINE_STATE_FUNCTIONS(main);
DEFINE_STATE_FUNCTIONS(landing);

static const struct smf_state states[] = {
    [PAD_STATE] = SMF_CREATE_STATE(pad_state_entry, pad_state_run, pad_state_exit),
    [COAST_STATE] = SMF_CREATE_STATE(coast_state_entry, coast_state_run, coast_state_exit),
    [APOGEE_STATE] = SMF_CREATE_STATE(apogee_state_entry, apogee_state_run, apogee_state_exit),
    [MAIN_STATE] = SMF_CREATE_STATE(main_state_entry, main_state_run, main_state_exit),
    [LANDING_STATE] = SMF_CREATE_STATE(landing_state_entry, landing_state_run, landing_state_exit),
};

struct s_object {
    struct smf_ctx ctx;
} state_obj;


static void pad_state_entry(void*) {

}

static void pad_state_run(void*) {

}

static void coast_state_entry(void*) {

}

static void coast_state_run(void*) {

}

static void apogee_state_entry(void*) {

}

static void apogee_state_run(void*) {

}

static void main_state_entry(void*) {

}

static void main_state_run(void*) {

}

static void landing_state_entry(void*) {

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
