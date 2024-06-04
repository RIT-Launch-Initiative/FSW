#include "data_reading.h"

#include "config.h"
#include "data_storage.h"
#include "ina260.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(data_read, CONFIG_APP_GRIM_REEFER_LOG_LEVEL);

bool flight_over = false;

// Forward Declares
static void adc_reading_task(void);
static void fast_reading_task(void);
static void slow_reading_task(void);

// Queues to storage
extern struct k_msgq adc_data_queue;
extern struct k_msgq fast_data_queue;
extern struct k_msgq slow_data_queue;

// Timers
K_TIMER_DEFINE(adc_timer, NULL, NULL);
K_TIMER_DEFINE(fast_timer, NULL, NULL);
K_TIMER_DEFINE(slow_timer, NULL, NULL);

// Events
#define BEGIN_READING_EVENT 1
K_EVENT_DEFINE(begin_reading);

// Threads
K_THREAD_DEFINE(adc_thread, 1024, adc_reading_task, NULL, NULL, NULL, ADC_READ_PRIORITY, 0, THREAD_START_DELAY);
K_THREAD_DEFINE(fast_thread, 1024, fast_reading_task, NULL, NULL, NULL, FAST_READ_PRIORITY, 0, THREAD_START_DELAY);
K_THREAD_DEFINE(slow_thread, 1024, slow_reading_task, NULL, NULL, NULL, SLOW_READ_PRIORITY, 0, THREAD_START_DELAY);
static int read_channel_to_float(const struct device* dev, enum sensor_channel chan, float* fval) {
    struct sensor_value val = {0};
    int ret = sensor_channel_get(dev, chan, &val);
    float v = sensor_value_to_float(&val);
    *fval = v;
    return ret;
}

static void adc_reading_task(void) {
    uint32_t adc_reading;
    struct adc_sequence sequence = {.buffer = &adc_reading, .buffer_size = sizeof(adc_reading)};
    struct adc_data data = {0};
    int count = 0;

    k_event_wait(&begin_reading, BEGIN_READING_EVENT, false, K_FOREVER);
    const struct data_devices* devs = (struct data_devices*) k_timer_user_data_get(&adc_timer);
    const struct adc_dt_spec* chan = devs->chan;
    LOG_INF("Adc chan %p", chan);

    while (true) {
        if (flight_over) {
            break;
        }
        k_timer_status_sync(&adc_timer);
        // Read ADC
        (void) adc_sequence_init_dt(chan, &sequence);
        adc_read_dt(chan, &sequence);
        data.adc_value[count] = adc_reading;
        count++;
        if (count == 10) {
            // send it
            k_msgq_put(&adc_data_queue, &data, K_NO_WAIT);
            count = 0;
            data.timestamp = k_uptime_get();
        }
    }
}
static void fast_reading_task(void) {
    k_event_wait(&begin_reading, BEGIN_READING_EVENT, false, K_FOREVER);
    const struct data_devices* devs = (struct data_devices*) k_timer_user_data_get(&fast_timer);
    const struct device* imu_dev = devs->fast.imu;
    struct fast_data data = {0};

    while (true) {
        k_timer_status_sync(&fast_timer);
        if (flight_over) {
            break;
        }
        // Read imu
        data.timestamp = k_uptime_get();
        sensor_sample_fetch(imu_dev);
        sensor_sample_fetch(devs->fast.altim);
        float x, y, z = 0;
        read_channel_to_float(imu_dev, SENSOR_CHAN_ACCEL_X, &x);
        read_channel_to_float(imu_dev, SENSOR_CHAN_ACCEL_Y, &y);
        read_channel_to_float(imu_dev, SENSOR_CHAN_ACCEL_Z, &z);
        data.acc.accel_x = x;
        data.acc.accel_y = y;
        data.acc.accel_z = z;

        read_channel_to_float(imu_dev, SENSOR_CHAN_GYRO_X, &x);
        read_channel_to_float(imu_dev, SENSOR_CHAN_GYRO_Y, &y);
        read_channel_to_float(imu_dev, SENSOR_CHAN_GYRO_Z, &z);
        data.gyro.gyro_x = x;
        data.gyro.gyro_y = y;
        data.gyro.gyro_z = z;
        read_channel_to_float(devs->fast.altim, SENSOR_CHAN_PRESS, &data.pressure);
        k_msgq_put(&fast_data_queue, &data, K_NO_WAIT);
    }
}
static void slow_reading_task(void) {
    k_event_wait(&begin_reading, BEGIN_READING_EVENT, false, K_FOREVER);
    const struct data_devices* devs = (struct data_devices*) k_timer_user_data_get(&slow_timer);
    struct slow_data data = {0};
    while (true) {
        if (flight_over) {
            break;
        }
        k_timer_status_sync(&slow_timer);
        // Read inas
        data.timestamp = k_uptime_get();
        sensor_sample_fetch(devs->slow.ina_adc);
        sensor_sample_fetch(devs->slow.ina_grim);
        sensor_sample_fetch(devs->slow.ina_bat);
        struct ina260_data* ina_data = devs->slow.ina_adc->data;
        struct ina260_data* grim_data = devs->slow.ina_grim->data;
        struct ina260_data* bat_data = devs->slow.ina_bat->data;
        read_channel_to_float(devs->slow.altim, SENSOR_CHAN_HUMIDITY, &data.humidity);
        read_channel_to_float(devs->slow.altim, SENSOR_CHAN_AMBIENT_TEMP, &data.temperature);

        data.grim_current = grim_data->current;
        data.grim_voltage = grim_data->v_bus;

        data.load_cell_current = ina_data->current;
        data.load_cell_voltage = ina_data->v_bus;

        data.bat_current = bat_data->current;
        data.bat_voltage = bat_data->v_bus;

        k_msgq_put(&slow_data_queue, &data, K_NO_WAIT);
    }
}

void start_data_reading(const struct data_devices* devs) {
    k_timer_user_data_set(&adc_timer, (void*) devs);
    k_timer_user_data_set(&fast_timer, (void*) devs);
    k_timer_user_data_set(&slow_timer, (void*) devs);

    k_timer_start(&adc_timer, ADC_DATA_DELAY, ADC_DATA_DELAY);
    k_timer_start(&fast_timer, FAST_DATA_DELAY, FAST_DATA_DELAY);
    k_timer_start(&slow_timer, SLOW_DATA_DELAY, SLOW_DATA_DELAY);
    k_event_set(&begin_reading, BEGIN_READING_EVENT);
}
void stop_data_reading() {
    flight_over = true;
    k_sleep(SLOW_DATA_DELAY); // Wait for all to quit
    LOG_INF("Stop data reading");
    k_timer_stop(&adc_timer);
    k_timer_stop(&fast_timer);
    k_timer_stop(&slow_timer);
}
