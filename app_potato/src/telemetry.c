// Self Include
#include "potato.h"

// Launch Includes
#include <launch_core/dev/sensor.h>
#include <launch_core/conversions.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#define TELEMETRY_STACK_SIZE 512

LOG_MODULE_REGISTER(telemetry);

// Threads
static void telemetry_read_task(void*);
K_THREAD_DEFINE(telem_read_thread, TELEMETRY_STACK_SIZE, telemetry_read_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0,
                1000);

static void telemetry_processing_task(void*);
K_THREAD_DEFINE(telem_process_thread, TELEMETRY_STACK_SIZE, telemetry_processing_task, NULL, NULL, NULL,
                K_PRIO_PREEMPT(20), 0,
                1000);

// Timers
K_TIMER_DEFINE(lps22_timer, NULL, NULL);

// Queues
K_MSGQ_DEFINE(raw_telem_processing_queue, sizeof(potato_raw_telemetry_t), 16, 1);

// Global Variables
float boost_detection_altitude = -0xFFFF;

// External Variables
extern struct k_msgq logging_queue;
extern bool logging_enabled;

static void convert_raw_telemetry(potato_raw_telemetry_t* raw_telem, potato_telemetry_t* telem) {
    telem->timestamp = raw_telem->timestamp;
    telem->altitude = l_altitude_conversion(raw_telem->lps22_data.pressure, raw_telem->lps22_data.temperature);

    // TODO: Update
    telem->load = raw_telem->load;
}

void configure_telemetry_rate(uint32_t frequency) {
    // TODO: Configure sensor speeds and timer

    bin_telemetry_file();
}

static void telemetry_read_task(void*) {
    // const struct device* lps22 = device_get_binding(DEVICE_DT_GET_ONE(st_lps22hhtr));
    const struct device* lps22 = NULL; // TODO: Fill DTS
    potato_raw_telemetry_t raw_telemetry = {0};

    k_timer_start(&lps22_timer, K_MSEC(100), K_MSEC(100));

    while (1) {
        k_timer_status_sync(&lps22_timer);

        sensor_sample_fetch(lps22);
        // TODO: Get ADC data
        raw_telemetry.timestamp = k_uptime_get_32();
        l_get_barometer_data_float(lps22, &raw_telemetry.lps22_data);

        k_msgq_put(&raw_telem_processing_queue, &raw_telem_processing_queue, K_NO_WAIT);
    }
}

static void telemetry_processing_task(void*) {
    potato_raw_telemetry_t raw_telemetry = {0};
    potato_telemetry_t processed_telemetry = {0};

    while (true) {
        if (k_msgq_get(&raw_telem_processing_queue, &raw_telemetry, K_FOREVER) != 0) continue;

        // Boost detection calculation. Can assume no boost if logging isn't enabled
        if (!logging_enabled) {
            convert_raw_telemetry(&raw_telemetry, &processed_telemetry);
            boost_detection_altitude = processed_telemetry.altitude;
        }

        // Buffer up data for logging before boost. If no space, throw out the oldest entry.
        if (!logging_enabled && k_msgq_num_free_get(&logging_queue) == 0) {
            potato_raw_telemetry_t throwaway_data;
            k_msgq_get(&logging_queue, &throwaway_data, K_NO_WAIT);
        }

        k_msgq_put(&logging_queue, &raw_telem_processing_queue, K_FOREVER);
    }
}
