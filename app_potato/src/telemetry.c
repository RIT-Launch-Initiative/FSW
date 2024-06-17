// Self Include
#include "potato.h"

// Launch Includes
#include <launch_core/conversions.h>
#include <launch_core/dev/sensor.h>

// Zephyr Includes
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define TELEMETRY_STACK_SIZE 512

LOG_MODULE_REGISTER(telemetry);

// Threads
static void telemetry_read_task(void*);
K_THREAD_DEFINE(telem_read_thread, TELEMETRY_STACK_SIZE, telemetry_read_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0,
                1000);

static void adc_read_task(void*);
K_THREAD_DEFINE(adc_read_thread, TELEMETRY_STACK_SIZE, adc_read_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

static void telemetry_processing_task(void*);
K_THREAD_DEFINE(telem_process_thread, TELEMETRY_STACK_SIZE, telemetry_processing_task, NULL, NULL, NULL,
                K_PRIO_PREEMPT(20), 0, 1000);

// Timers
K_TIMER_DEFINE(lps22_timer, NULL, NULL);
K_TIMER_DEFINE(adc_timer, NULL, NULL);

// Queues
#define ADC_QUEUE_SIZE 100
K_MSGQ_DEFINE(raw_telem_processing_queue, sizeof(potato_raw_telemetry_t), 16, 1);
K_MSGQ_DEFINE(adc_telem_processing_queue, sizeof(potato_adc_telemetry_t), ADC_QUEUE_SIZE, 1);

// Global Variables
float boost_detection_altitude = -0xFFFF;

// External Variables
extern struct k_msgq logging_queue;
extern bool logging_enabled;

static void convert_raw_telemetry(potato_raw_telemetry_t* raw_telem, potato_telemetry_t* telem) {
    telem->timestamp = raw_telem->timestamp;
    telem->altitude = l_altitude_conversion(raw_telem->lps22_data.pressure, raw_telem->lps22_data.temperature);
}

void configure_telemetry_rate(uint32_t frequency) {
    // TODO: Configure sensor speeds and timer

    bin_telemetry_file();
}

static void telemetry_read_task(void*) {

    const struct device* lps22 = DEVICE_DT_GET(DT_NODELABEL(lps22hh));
    // potato_raw_telemetry_t raw_telemetry = {0};

    // k_timer_start(&lps22_timer, K_MSEC(100), K_MSEC(100));
    sensor_sample_fetch(lps22);

    // while (1) {
    //     k_timer_status_sync(&lps22_timer);
    //
    //     sensor_sample_fetch(lps22);
    //     // TODO: Get ADC data
    //     raw_telemetry.timestamp = k_uptime_get_32();
    //     l_get_barometer_data_float(lps22, &raw_telemetry.lps22_data);
    //
    //     k_msgq_put(&raw_telem_processing_queue, &raw_telem_processing_queue, K_NO_WAIT);
    // }
}
static void adc_read_task(void*) {
    static const struct adc_dt_spec adc_chan0 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

    int32_t buf = 0;
    struct adc_sequence sequence = {
        .buffer = &buf,
        .buffer_size = sizeof(buf),
    };
    sequence.channels = adc_chan0.channel_id;
    potato_adc_telemetry_t adc_data = {0};

    int err = adc_sequence_init_dt(&adc_chan0, &sequence);
    if (err < 0) {
        LOG_ERR("Could not init adc channel sequence: %d", err);
    }

    k_timer_start(&adc_timer, ADC_PERIOD, ADC_PERIOD);

    int i = 0;

    while (1) {
        k_timer_status_sync(&adc_timer);

        if (i == 0) {
            adc_data.timestamp = k_uptime_get();
        }
        err = adc_read_dt(&adc_chan0, &sequence);
        if (err < 0) {
            LOG_ERR("Could not read adc chan0 (%d)\n", err);
            continue;
        }

        ASSIGN_V32_TO_ADCDATA(buf, adc_data.data[i]);
        i++;

        if (i == ADC_READINGS_PER_PACKET) {
            k_msgq_put(&adc_telem_processing_queue, &adc_data, K_NO_WAIT);
            i = 0;
        }
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
