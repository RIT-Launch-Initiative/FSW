#include "control.h"
#include "config.h"
#include "adc_reading.h"
#include "flash_storage.h"
#include "buzzer.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <stdbool.h>
#include <stdint.h>

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

    LOG_INF("Starting test run");
    test_running = true;

    start_flash_storage();
    adc_start_reading();

    test_number++;
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

void control_print_one(const struct shell *shell){
    set_ldo(1);
    k_msleep(5000);
    uint32_t adc_val = 0;
    adc_read_one(&adc_val);
    shell_print(shell, "%d", adc_val);
    set_ldo(0);
}

void control_dump_data(const struct shell *shell){
    flash_dump_all(shell);
}

void control_dump_one(const struct shell *shell, uint32_t test_index){
    flash_dump_one(shell, test_index);
}

void control_erase_all(const struct shell *shell){
    flash_erase_all(shell);
}

bool control_is_running(){
    return test_running;
}