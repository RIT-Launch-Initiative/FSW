#include "pump.h"

#include "5v_ctrl.h"
#include "common.h"
#include "f_core/utils/linear_fit.hpp"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pump);

using FIRFilter = CMovingAverage<float, 4>;

int64_t last_pump_on_time = 0;
int64_t last_pump_off_time = 0;

const int64_t pump_cooldown_ms = 20000;
const int64_t pump_maxon_ms = 10000;
const float off_threshhold = 0.51;
FIRFilter filt{0};

bool should_pump_be_on(int64_t now, bool on, float volts, float current) {
    int32_t time_since_off = now - last_pump_off_time;
    int32_t time_since_on = now - last_pump_on_time;
    filt.Feed(current);
    printk("%d, %d, %f\n", time_since_off, time_since_on, (double) filt.Avg());
    // Shutdown for pumps (duty cycle)
    if (on && (time_since_off > pump_maxon_ms)) {
        // reset filter
        LOG_INF("Duty cycle shutdown");
        return false;
    }
    if (filt.Avg() > off_threshhold) {
        LOG_INF("Current draw shutdown");
        return false;
    }
    // Starting the pumps back up
    if (!on && (time_since_on > pump_cooldown_ms)) {
        LOG_INF("Starting now");
        return true;
    }
    LOG_INF("Not Commanded otherwise");
    return on;
};

K_TIMER_DEFINE(power_timer, NULL, NULL);

int attempt_inflation_iteration(const struct device *ina_pump) {
    k_timer_start(&power_timer, K_MSEC(100), K_MSEC(100));

    bool state = false;
    while (true) {
        k_timer_status_sync(&power_timer);
        float volts = 0;
        float current = 0;

        read_ina(ina_pump, volts, current);
        int64_t now = k_uptime_get();
        // inputs print
        printk("%lld, %d, %f, %f, ", now, (int) state, (double) volts, (double) current);
        state = should_pump_be_on(now, state, volts, current);
        if (state) {
            last_pump_on_time = now;
        } else {
            last_pump_off_time = now;
        }
        rail_item_set(FiveVoltItem::Pump, state);
    }
}