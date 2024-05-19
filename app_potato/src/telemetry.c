// Self Include
#include "potato.h"

// Launch Includes
#include <launch_core/dev/sensor.h>
#include <launch_core/conversions.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#define TELEMETRY_STACK_SIZE 512

static void sensor_read_task(void *);
K_THREAD_DEFINE(sensor_read_thread, TELEMETRY_STACK_SIZE, sensor_read_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

K_TIMER_DEFINE(lps22_timer, NULL, NULL);

static convert_raw_telemetry(potato_raw_telemetry_t &raw_telem, potato_telemetry_t &telem) {
    telem.timestamp = raw_telem.timestamp;
    telem.altitude = l_altitude_conversion(raw_telem.lps22_data.pressure, raw_telem.lps22_data.temperature);

    // TODO: Update
    telem.load = raw_telem.load;
}

static void sensor_read_task(void *) {
    const struct device *lps22 = device_get_binding(DEVICE_DT_GET_ONE(st_lps22hhtr));
    potato_raw_telemetry_t raw_telemetry = {0};
    potato_telemetry_t processed_telemetry = {0};

    k_timer_start(&lps22_timer, K_MSEC(100), K_MSEC(100));

    while (1) {
        k_timer_status_sync(&lps22);

        sensor_sample_fetch(lps22);
        // TODO: Get ADC data
        raw_telemetry.timestamp = k_uptime_get_32();
        l_get_barometer_data_float(lps22, &raw_telemetry.lps22_data);

        convert_raw_telemetry(raw_telemetry, processed_telemetry);
    }
}
