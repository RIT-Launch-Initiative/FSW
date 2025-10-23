#include "control.h"
#include "config.h"
#include "adc_reading.h"
#include "flash_storage.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <stdbool.h>

LOG_MODULE_REGISTER(control, LOG_LEVEL_INF);

static int test_number = 0;
static bool test_running = false;

void control_init(){
    LOG_INF("Initializing...");
    adc_init();
}

void control_start_test(){
    if(test_running){
        LOG_WRN("Test already running");
        return;
    }

    LOG_INF("Starting test run #%d", test_number + 1);
    test_running = true;

    start_flash_storage();
    adc_start_reading();

    test_number++;

    // Run test for 10 seconds
    k_sleep(K_SECONDS(TEST_DURATION));
    control_stop_test();
}

void control_stop_test(){
    if(!test_running){
        LOG_WRN("No test running");
        return;
    }

    LOG_INF("Stopping test...");
    adc_stop_recording();
    stop_flash_storage();
    test_running = false;
}

void control_dump_data(const struct shell *shell){
    flash_dump_all(shell);
}

bool control_is_running(){
    return test_running;
}