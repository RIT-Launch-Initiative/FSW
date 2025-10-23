#include "adc_reading.h"
#include "config.h"
#include "flash_storage.h"

#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

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

K_THREAD_DEFINE(adc_thread, 1024, adc_reading_task, NULL, NULL, NULL, ADC_READ_PRIORITY, 0, THREAD_START_DELAY);

static uint32_t adc_buffer;
static struct adc_sequence sequence = {
    .buffer = &adc_buffer,
    .buffer_size = sizeof(adc_buffer),
    .resolution = 24
};

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
    ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

static const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
};

int adc_init(){
    if(!adc_is_ready_dt(&adc_channels[0])){
        LOG_ERR("ADC controller device %s not ready\n", adc_channels[0].dev->name);
        return 0;
    }

    int err = adc_channel_setup_dt(&adc_channels[0]);
    if(err < 0){
        LOG_ERR("Could not setup channel (%d)\n", err);
        return 0;
    }

    LOG_INF("ADC initialized");
    return 0;
}

void adc_reading_task(){
    int ret;
    uint32_t adc_val = 0;
    struct adc_sample sample = {0};

    LOG_INF("ADC reading task waiting to start...");

    // Wait for start event
    k_event_wait(&adc_control_event, BEGIN_READING_EVENT, true, K_FOREVER);

    LOG_INF("ADC reading started");

    k_timer_start(&adc_timer, K_MSEC(1), K_MSEC(1)); // 1kHz loop

    int x = 0;

    while(true){
        uint32_t events = k_event_wait(&adc_control_event, BEGIN_READING_EVENT | STOP_READING_EVENT, false, K_NO_WAIT);
        if(events & STOP_READING_EVENT){
            break;
        }

        k_timer_status_sync(&adc_timer);

        sequence.buffer = &adc_val;
        sequence.buffer_size = sizeof(adc_val);

        // Read from ADC
        (void)adc_sequence_init_dt(&adc_channels[0], &sequence);
        ret = adc_read(adc_dev, &sequence);
        if(ret < 0){
            LOG_ERR("ADC read failed (%d)", ret);
            continue;
        }

        sample.timestamp = k_uptime_get_32();
        sample.value = adc_val;

        if(x % 100 == 0){
            LOG_INF("sample value: %u", sample.value);
        }
        x++;

        k_msgq_put(&adc_data_queue, &sample, K_NO_WAIT);
    }

    k_timer_stop(&adc_timer);
}

void adc_start_reading(){
    k_event_set(&adc_control_event, BEGIN_READING_EVENT);
}

void adc_stop_recording(){
    k_event_set(&adc_control_event, STOP_READING_EVENT);
}