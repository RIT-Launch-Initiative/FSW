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

const float off_threshhold = 0.51;
FIRFilter current_filt{0};

bool should_pump_be_on(int64_t now, float volts, float current) {
    int32_t time_since_off = now - last_pump_off_time;
    current_filt.Feed(current);
    // printk("%d, %f\n", time_since_off, (double) current_filt.Avg());

    // Shutdown for pumps (duty cycle)
    if (time_since_off > PUMP_DUTY_ON_MS) {
        LOG_INF("Duty cycle shutdown");
        return false;
    }
    if (current_filt.Avg() > off_threshhold) {
        LOG_INF("Current draw shutdown");
        return false;
    }
    // LOG_INF("Not Commanded otherwise");
    return true;
};

K_TIMER_DEFINE(power_timer, NULL, NULL);

int attempt_inflation_iteration(const struct device *ina_pump) {
    int64_t iteration_start_ms = k_uptime_get();

    k_timer_start(&power_timer, K_MSEC(100), K_MSEC(100));

    current_filt.Fill(0);
    LOG_INF("Start Pump");
    int64_t now = k_uptime_get();
    last_pump_off_time = now;
    rail_item_set(FiveVoltItem::Pump, true);
    while (k_uptime_get() < iteration_start_ms + PUMP_DUTY_ON_MS) {
        k_timer_status_sync(&power_timer);
        float volts = 0;
        float current = 0;

        read_ina(ina_pump, volts, current);
        // inputs print
        // printk("%lld, %f, %f, ", now, (double) volts, (double) current);
        if (!should_pump_be_on(now, volts, current)) {
            break;
        }
    }
    LOG_INF("Done pumping");
    rail_item_set(FiveVoltItem::Pump, false);

    return 0;
}