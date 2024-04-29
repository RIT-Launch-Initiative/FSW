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

    const struct adc_dt_spec vin_sense_adc = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

    struct adc_sequence vin_sense_sequence = {
        .buffer = &temp_vin_adc_data,
        .buffer_size = sizeof(temp_vin_adc_data),
    };

    const bool ina_device_found[] = {l_check_device(sensors[0]) == 0, l_check_device(sensors[1]) == 0,
                                     l_check_device(sensors[2]) == 0};

    const bool adc_ready = l_init_adc_channel(&vin_sense_adc, &vin_sense_sequence) == 0;

    if (!ina_device_found[0]) {
        LOG_ERR("INA219 battery sensor not found");
    }

    if (!ina_device_found[1]) {
        LOG_ERR("INA219 3v3 sensor not found");
    }

    if (!ina_device_found[2]) {
        LOG_ERR("INA219 5v0 sensor not found");
    }

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

        struct sensor_value current;
        struct sensor_value voltage;
        struct sensor_value power;
        struct sensor_value *sensor_values[] = {&current, &voltage, &power};

        if (likely(ina_device_found[0])) {
            l_get_sensor_data(sensors[0], 3, ina_channels, sensor_values);
            sensor_telemetry.data_battery.current = sensor_value_to_float(&current);
            sensor_telemetry.data_battery.voltage = sensor_value_to_float(&voltage);
            sensor_telemetry.data_battery.power = sensor_value_to_float(&power);
        }

        if (likely(ina_device_found[1])) {
            l_get_sensor_data(sensors[1], 3, ina_channels, sensor_values);
            sensor_telemetry.data_3v3.current = sensor_value_to_float(&current);
            sensor_telemetry.data_3v3.voltage = sensor_value_to_float(&voltage);
            sensor_telemetry.data_3v3.power = sensor_value_to_float(&power);
        }

        if (likely(ina_device_found[2])) {
            l_get_sensor_data(sensors[2], 3, ina_channels, sensor_values);
            sensor_telemetry.data_5v0.current = sensor_value_to_float(&current);
            sensor_telemetry.data_5v0.voltage = sensor_value_to_float(&voltage);
            sensor_telemetry.data_5v0.power = sensor_value_to_float(&power);
        }

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