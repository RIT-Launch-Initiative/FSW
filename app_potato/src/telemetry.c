// Self Include
#include "potato.h"

// Launch Includes
#include <launch_core/conversions.h>
#include <launch_core/dev/sensor.h>

// Zephyr Includes
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/adc_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define TELEMETRY_STACK_SIZE 512

LOG_MODULE_REGISTER(telemetry);

// Threads
static void telemetry_read_task(void *);
K_THREAD_DEFINE(telem_read_thread, TELEMETRY_STACK_SIZE, telemetry_read_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0,
                1000);

static void adc_read_task(void *);
K_THREAD_DEFINE(adc_read_thread, TELEMETRY_STACK_SIZE, adc_read_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

static void telemetry_processing_task(void *);
K_THREAD_DEFINE(telem_process_thread, TELEMETRY_STACK_SIZE, telemetry_processing_task, NULL, NULL, NULL,
                K_PRIO_PREEMPT(20), 0, 1000);

// Timers
K_TIMER_DEFINE(lps22_timer, NULL, NULL);
K_TIMER_DEFINE(adc_timer, NULL, NULL);

// Queues
extern struct k_msgq logging_queue;
extern struct k_msgq adc_logging_queue;
K_MSGQ_DEFINE(boost_queue, sizeof(potato_raw_telemetry_t), 500, 1);

// Global Variables
float boost_detection_altitude = -0xFFFF;

// External Variables
extern struct k_msgq logging_queue;
extern bool logging_enabled;
extern bool boost_detected;

// static void convert_raw_telemetry(potato_raw_telemetry_t* raw_telem, potato_telemetry_t* telem) {
// telem->timestamp = raw_telem->timestamp;
// telem->altitude = l_altitude_conversion(raw_telem->lps22_data.pressure, raw_telem->lps22_data.temperature);
// }

void configure_telemetry_rate(uint32_t frequency) {
    // TODO: Configure sensor speeds and timer

    bin_telemetry_file();
}

void read_lps(potato_raw_telemetry_t *telem, const struct device *dev) {
#ifdef CONFIG_BOARD_NATIVE_SIM
    telem->lps22_data.pressure = 50;
#else
    sensor_sample_fetch(dev);
    l_get_barometer_data_float(dev, &telem->lps22_data);
#endif
}

static void telemetry_read_task(void *) {
    potato_raw_telemetry_t raw_telemetry = {0};
#ifdef CONFIG_BOARD_POTATO
    const struct device *lps22 = DEVICE_DT_GET(DT_NODELABEL(lps22hh));
#else
    const struct device *lps22 = NULL;
#endif
    LOG_INF("Sensor Reader Ready");

    // Start firing immediatly, we're needed for boost detect
    k_timer_start(&lps22_timer, K_MSEC(100), K_MSEC(100));
    while (!boost_detected) {
        k_timer_status_sync(&lps22_timer);

        read_lps(&raw_telemetry, lps22);
        raw_telemetry.timestamp = k_uptime_get();

        k_msgq_put(&boost_queue, &raw_telemetry, K_NO_WAIT);
    }
    while (logging_enabled) {
        k_timer_status_sync(&lps22_timer);

        read_lps(&raw_telemetry, lps22);
        raw_telemetry.timestamp = k_uptime_get();

        k_msgq_put(&logging_queue, &raw_telemetry, K_NO_WAIT);
    }
}
static void adc_read_task(void *) {
    static const struct adc_dt_spec adc_chan0 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
    int32_t buf = 0;
    struct adc_sequence sequence = {
        .buffer = &buf,
        .buffer_size = sizeof(buf),
    };
    sequence.channels = adc_chan0.channel_id;
    sequence.resolution = 16; // needed for adc_emul. adc_init_sequence overwrites in normal circumstances
    potato_adc_telemetry_t adc_data = {0};

    int err = adc_sequence_init_dt(&adc_chan0, &sequence);
    if (err < 0) {
        LOG_ERR("Could not init adc channel sequence: %d", err);
    }

    LOG_INF("ADC Reader Ready");
    // ADC isn't needed for boost detection
    SPIN_WHILE(!boost_detected, 1);

    k_timer_start(&adc_timer, ADC_PERIOD, ADC_PERIOD);
    //
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
#ifdef CONFIG_BOARD_NATIVE_SIM
        adc_data.data[i][0] = 0xff;
#endif
        i++;

        if (i == ADC_READINGS_PER_PACKET) {
            k_msgq_put(&adc_logging_queue, &adc_data, K_NO_WAIT);
            i = 0;
        }
    }
}
double my_altitude_conversion(double pressure_kpa, double temperature_c) {
    double pressure = pressure_kpa * 10;
    double altitude = (1 - pow(pressure / 1013.25, 0.190284)) * 145366.45 * 0.3048;
    return altitude;
}
static void telemetry_processing_task(void *) {
    potato_raw_telemetry_t telemetry = {0};
    //
    while (!boost_detected) {
        if (k_msgq_get(&boost_queue, &telemetry, K_MSEC(1)) != 0) {
            // some error
            continue;
        }
        // If boost queue is receiving data, can assume no boost has happened
        float altitude = my_altitude_conversion(telemetry.lps22_data.pressure, telemetry.lps22_data.temperature);
        boost_detection_altitude = altitude;
        //
        // // Buffer up data for logging before boost.
        //If no space, throw out the oldest entry.
        if (k_msgq_num_free_get(&boost_queue) == 0) {
            potato_raw_telemetry_t throwaway_data;
            k_msgq_get(&logging_queue, &throwaway_data, K_NO_WAIT);
        }
        // Add next sample
        k_msgq_put(&logging_queue, &telemetry, K_FOREVER);
    }
}
