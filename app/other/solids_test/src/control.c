#include "control.h"
#include "adc_reading.h"
#include "buzzer.h"
#include "config.h"
#include "flash_storage.h"

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(control, LOG_LEVEL_INF);

static bool test_running = false;

int control_init(void) {
    LOG_INF("Initializing...");
    return adc_init();
}

void control_start_test() {
    if (test_running) {
        LOG_WRN("Test already running");
        return;
    }

    test_running = true;

    start_flash_storage();
    adc_start_reading();
}

void control_stop_test() {
    if (!test_running) {
        LOG_WRN("No test running");
        return;
    }

    LOG_INF("Stopping test...");
    adc_stop_recording();
    stop_flash_storage();
    test_running = false;
}

void control_print_n(const struct shell *shell, int num) {
    // set_ldo(1);
    k_msleep(5000);
    uint32_t adc_val = 0;
    uint32_t start = k_uptime_ticks();
    for (int i = 0; i < num; i++) {

        adc_read_one(&adc_val);
        uint32_t t = k_uptime_ticks() - start;
        shell_print(shell, "%u, %d", k_ticks_to_us_near32(t), adc_val);
        k_msleep(1);
    }
    // set_ldo(0);
}

void control_dump_data(const struct shell *shell) { flash_dump_all(shell); }

void control_dump_one(const struct shell *shell, uint32_t test_index) { flash_dump_one(shell, test_index); }

void control_erase_all(const struct shell *shell) { flash_erase_all(shell); }

bool control_is_running() { return test_running; }

void control_set_ematch(const struct shell *shell) {
    set_ematch(1);
    shell_print(shell, "Ematch: 1");
}

void control_stop_ematch(const struct shell *shell) {
    set_ematch(0);
    shell_print(shell, "Ematch: 0");
}