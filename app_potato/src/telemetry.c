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
// static void telemetry_read_task(void*);
// K_THREAD_DEFINE(telem_read_thread, TELEMETRY_STACK_SIZE, telemetry_read_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0,
// 1000);

static void adc_read_task(void*);
K_THREAD_DEFINE(adc_read_thread, TELEMETRY_STACK_SIZE, adc_read_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

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

static void adc_read_task(void*) {
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
#ifdef CONFIG_DEBUG
    // SPIN_WHILE(!boost_detected, 1); // TODO(aaron)
#endif
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

#ifdef CONFIG_BOARD_NATIVE_SIM
        adc_data.data[i].parts[0] = 0xff;
#endif
        i++;

        if (i == ADC_READINGS_PER_PACKET) {
            k_msgq_put(&adc_logging_queue, &adc_data, K_NO_WAIT);
            i = 0;
        }

        k_sleep(K_SECONDS(1));
    }
}
