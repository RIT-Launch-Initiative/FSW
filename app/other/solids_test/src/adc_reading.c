#include "adc_reading.h"
#include "buzzer.h"
#include "config.h"
#include "flash_storage.h"
#include "control.h"

#include <stdint.h>
#include <stdio.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(adc_reader, LOG_LEVEL_INF);

#define ADC_NODE DT_ALIAS(adc0)

#if !DT_NODE_HAS_STATUS(ADC_NODE, okay)
#error "No ADC node found"
#endif

static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);

extern struct k_msgq adc_data_queue;

#define BEGIN_READING_EVENT 1
#define STOP_READING_EVENT  2

static K_EVENT_DEFINE(adc_control_event);
static K_TIMER_DEFINE(adc_timer, NULL, NULL);

void adc_reading_task(void);

K_THREAD_DEFINE(adc_thread, 1024, adc_reading_task, NULL, NULL, NULL, 15, 0, THREAD_START_DELAY);

static uint32_t adc_buffer;
static struct adc_sequence sequence = {.buffer = &adc_buffer, .buffer_size = sizeof(adc_buffer), .resolution = 24};

#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

static const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};

int adc_init(void) {
    if (!adc_is_ready_dt(&adc_channels[0])) {
        LOG_ERR("ADC controller device %s not ready\n", adc_channels[0].dev->name);
        return -1;
    }

    int err = adc_channel_setup_dt(&adc_channels[0]);
    if (err < 0) {
        LOG_ERR("Could not setup channel (%d)\n", err);
        return -1;
    }

    (void) adc_sequence_init_dt(&adc_channels[0], &sequence);

    LOG_INF("ADC initialized");
    return 0;
}

// Read one adc sample
void adc_read_one(uint32_t *adc_val) {
    sequence.buffer = adc_val;
    sequence.buffer_size = sizeof(*adc_val);

    int ret = adc_read(adc_dev, &sequence);
    if (ret < 0) {
        LOG_ERR("ADC read failed (%d)", ret);
        return;
    }
}

void adc_reading_task() {
    uint32_t adc_val = 0;
    struct adc_sample sample = {0};
    while (true) {
        LOG_INF("ADC reading task waiting to start...");

        // Wait for start event
        k_event_wait(&adc_control_event, BEGIN_READING_EVENT, true, K_FOREVER);
        LOG_INF("ADC reading started");
        // set_ldo(1);
        // Delay test 2 seconds, beep when test actually starts
        k_msleep(2000);
        test_start_beep();

        k_timer_start(&adc_timer, K_USEC(SAMPLE_RATE_HZ), K_USEC(SAMPLE_RATE_HZ)); // 1000Hz periods

        int x = 0;
        uint32_t dropped_samples = 0;
        uint64_t total_adc_ticks = 0;
        uint64_t total_loop_ticks = 0;
        uint32_t num_missed_expires = 0;
        uint64_t start_time_ticks = k_uptime_ticks();
        
        while (true) {
            uint32_t start_loop_ticks = k_uptime_ticks();
            uint32_t events =
                k_event_wait(&adc_control_event, BEGIN_READING_EVENT | STOP_READING_EVENT, false, K_NO_WAIT);
            if (events & STOP_READING_EVENT) {
                break;
            }

            uint32_t num_expiries = k_timer_status_get(&adc_timer);
            if (num_expiries == 0) {
                k_timer_status_sync(&adc_timer);
            } else {
                num_missed_expires += num_expiries - 1;
            }

            // Set ematch 500ms into test
            if (x == 500) {
                set_ematch(1);
            } else if (x == 900) {
                set_ematch(0);
            }

            // Read from ADC
            uint32_t start_read = k_uptime_ticks();
            adc_read_one(&adc_val);
            total_adc_ticks += k_uptime_ticks() - start_read;

            sample.timestamp = k_ticks_to_us_near32(k_uptime_ticks() - start_time_ticks);
            sample.value = adc_val;
            
            if (k_msgq_put(&adc_data_queue, &sample, K_NO_WAIT) != 0) {
                dropped_samples++;
            }
            x++;
            total_loop_ticks += k_uptime_ticks() - start_loop_ticks;

            // Stop test after 10 seconds
            if ((k_ticks_to_ms_near32(k_uptime_ticks()) - k_ticks_to_ms_near32(start_time_ticks)) >= TEST_DURATION) {
                control_stop_test();
                break;
            }
        }

        // set_ldo(0);
        k_timer_stop(&adc_timer);
        LOG_INF(
            "number of samples: %d, %u missed, %u dropped, read time %llu, ms per = %.2f, loop time: %llu, loop time ticks: %llu",
            x, num_missed_expires, dropped_samples, k_ticks_to_ms_near64(total_adc_ticks),
            (double) k_ticks_to_ms_near64(total_adc_ticks) / (double) x, k_ticks_to_ms_near64(total_loop_ticks),
            total_loop_ticks);
        test_end_beep();
    }
}

void adc_start_reading() { k_event_set(&adc_control_event, BEGIN_READING_EVENT); }

void adc_stop_recording() { k_event_set(&adc_control_event, STOP_READING_EVENT); }