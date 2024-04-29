#include <launch_core/dev/adc.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/dev/sensor.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

#define INA219_UPDATE_TIME_MS       (67)
#define SENSOR_READ_STACK_SIZE      (640)


//    k_thread_create(&ina_read_thread, &ina_read_stack[0], SENSOR_READ_STACK_SIZE, ina_task, NULL, NULL, NULL,
//                    K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
//    k_thread_start(&ina_read_thread);

LOG_MODULE_REGISTER(telemetry);

static void ina_task(void *, void *, void *);

K_THREAD_DEFINE(ina_thread, SENSOR_READ_STACK_SIZE, ina_task, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0, 1000);

extern struct k_msgq ina_processing_queue;

static void init_ina_task(const struct device *sensors[3], bool ina_device_found[3]) {
    const char *sensor_names[] = {"Battery", "3v3", "5v0"};

    for (int i = 0; i < 3; i++) {
        ina_device_found[i] = l_check_device(sensors[i]) == 0;
        if (!ina_device_found[i]) {
            LOG_ERR("INA219 %s sensor not found", sensor_names[i]);
        }
    }
}

static void ina_task(void *, void *, void *) {
    static const enum sensor_channel ina_channels[] = {SENSOR_CHAN_CURRENT, SENSOR_CHAN_VOLTAGE, SENSOR_CHAN_POWER};

    power_module_telemetry_t sensor_telemetry = {0};
    float vin_adc_data_mv = 0;
    uint16_t temp_vin_adc_data = 0;

    const struct device *sensors[] = {
            DEVICE_DT_GET(DT_ALIAS(inabatt)), // Battery
            DEVICE_DT_GET(DT_ALIAS(ina3v3)),  // 3v3
            DEVICE_DT_GET(DT_ALIAS(ina5v0))   // 5v0
    };

    bool ina_device_found[3] = {false};
    init_ina_task(sensors, ina_device_found);


    const struct adc_dt_spec vin_sense_adc = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

    struct adc_sequence vin_sense_sequence = {
            .buffer = &temp_vin_adc_data,
            .buffer_size = sizeof(temp_vin_adc_data),
    };

    const bool adc_ready = l_init_adc_channel(&vin_sense_adc, &vin_sense_sequence) == 0;


    if (!adc_ready) {
        LOG_ERR("ADC channel %d is not ready", vin_sense_adc.channel_id);
        sensor_telemetry.vin_adc_data_v = -0x7FFF;
    }

    while (true) {
        l_update_sensors_safe(sensors, 3, ina_device_found);
        if (likely(adc_ready)) {
            if (0 <= l_read_adc_mv(&vin_sense_adc, &vin_sense_sequence, (int32_t *) &vin_adc_data_mv)) {
                sensor_telemetry.vin_adc_data_v = (vin_adc_data_mv * MV_TO_V_MULTIPLIER) * ADC_GAIN;
            } else {
                LOG_ERR("Failed to read ADC value from %d", vin_sense_adc.channel_id);
                sensor_telemetry.vin_adc_data_v = -0x7FFF;
            }
        }
        sensor_telemetry.timestamp = k_uptime_get_32();

        l_get_shunt_data_float(sensors[0], &sensor_telemetry.data_battery);
        l_get_shunt_data_float(sensors[1], &sensor_telemetry.data_3v3);
        l_get_shunt_data_float(sensors[2], &sensor_telemetry.data_5v0);

        if (k_msgq_put(&ina_processing_queue, &sensor_telemetry, K_NO_WAIT)) {
            LOG_ERR("Failed to put data into INA219 processing queue");
        }

        // Wait some time for sensor to get new values (15 Hz -> 66.67 ms)
        uint32_t time_to_wait = INA219_UPDATE_TIME_MS - (k_uptime_get_32() - sensor_telemetry.timestamp);
        if (time_to_wait > 0) {
            k_sleep(K_MSEC(time_to_wait));
        }
    }
}