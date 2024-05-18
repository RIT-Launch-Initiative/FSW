/*
 * Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "power_module.h"

#include <launch_core/dev/adc.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/dev/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define INA219_UPDATE_TIME_MS  (67)
#define ADC_UPDATE_TIME_MS     (15)
#define SENSOR_READ_STACK_SIZE (1024)

LOG_MODULE_REGISTER(telemetry);

// Threads
K_THREAD_DEFINE(ina_thread, SENSOR_READ_STACK_SIZE, ina_task, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0, 1000);
K_THREAD_DEFINE(adc_thread, SENSOR_READ_STACK_SIZE, adc_task, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0, 1000);

// Message Queues
K_MSGQ_DEFINE(ina_telemetry_msgq, sizeof(power_module_telemetry_t), 10, 4);
K_MSGQ_DEFINE(adc_telemetry_msgq, sizeof(float), 20, 4);

// Timers
K_TIMER_DEFINE(ina_task_timer, NULL, NULL);
K_TIMER_DEFINE(adc_task_timer, NULL, NULL);

// Extern variables
extern bool logging_enabled;
extern struct k_msgq ina_logging_msgq;
extern struct k_msgq adc_logging_msgq;

static bool init_ina_task(const struct device *sensors[3], bool ina_device_found[3]) {
    const char *sensor_names[] = {"Battery", "3v3", "5v0"};
    int error_count = 0;

    for (int i = 0; i < 3; i++) {
        ina_device_found[i] = l_check_device(sensors[i]) == 0;
        if (!ina_device_found[i]) {
            LOG_ERR("INA219 %s sensor not found", sensor_names[i]);
            error_count++;
        }
    }

    if (error_count == 3) {
        return false;
    }

    k_timer_start(&ina_task_timer, K_MSEC(INA219_UPDATE_TIME_MS), K_MSEC(INA219_UPDATE_TIME_MS));
    return true;
}

static bool init_adc_task(const struct adc_dt_spec *p_vin_sense_adc, struct adc_sequence *p_vin_sense_sequence) {
    const bool adc_ready = l_init_adc_channel(p_vin_sense_adc, p_vin_sense_sequence) == 0;

    if (!adc_ready) {
        LOG_ERR("ADC channel %d is not ready", p_vin_sense_adc->channel_id);
    } else {
        k_timer_start(&adc_task_timer, K_MSEC(ADC_UPDATE_TIME_MS), K_MSEC(ADC_UPDATE_TIME_MS));
    }

    return adc_ready;
}

void ina_task(void) {
    const struct device *sensors[] = {
        DEVICE_DT_GET(DT_ALIAS(inabatt)), // Battery
        DEVICE_DT_GET(DT_ALIAS(ina3v3)),  // 3v3
        DEVICE_DT_GET(DT_ALIAS(ina5v0))   // 5v0
    };
    static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

    bool ina_device_found[3] = {false};
    power_module_telemetry_t sensor_telemetry = {0};

    if (!init_ina_task(sensors, ina_device_found)) {
        return;
    }

    while (true) {
        k_timer_status_sync(&ina_task_timer); // TODO: Confirm timing
        gpio_pin_toggle_dt(&led);

        l_update_sensors_safe(sensors, 3, ina_device_found);

        l_get_shunt_data_float(sensors[0], &sensor_telemetry.data_battery);
        l_get_shunt_data_float(sensors[1], &sensor_telemetry.data_3v3);
        l_get_shunt_data_float(sensors[2], &sensor_telemetry.data_5v0);

        if (k_msgq_put(&ina_telemetry_msgq, &sensor_telemetry, K_NO_WAIT)) {
        }

        // Buffer up data for logging before boost. If no space, throw out the oldest entry.
        if (!logging_enabled && k_msgq_num_free_get(&ina_logging_msgq) == 0) {
            power_module_telemetry_t throwaway_data;
            k_msgq_get(&ina_logging_msgq, &throwaway_data, K_NO_WAIT);
        }

        if (k_msgq_put(&ina_logging_msgq, &sensor_telemetry, K_MSEC(INA219_UPDATE_TIME_MS))) {
        }
    }
}

void adc_task(void) {
    static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
    static const float adc_gain = 0.09f;
    static const float mv_to_v_multiplier = 0.001f;
    const struct adc_dt_spec vin_sense_adc = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

    float vin_adc_data_mv = 0;
    uint16_t temp_vin_adc_data = 0;

    struct adc_sequence vin_sense_sequence = {
        .buffer = &temp_vin_adc_data,
        .buffer_size = sizeof(temp_vin_adc_data),
    };

    if (!init_adc_task(&vin_sense_adc, &vin_sense_sequence)) {
        return;
    }

    while (true) {
        k_timer_status_sync(&adc_task_timer);
        gpio_pin_toggle_dt(&led);

        if (0 < l_read_adc_mv(&vin_sense_adc, &vin_sense_sequence, (int32_t *) &vin_adc_data_mv)) {
            LOG_ERR("Failed to read ADC value from %d", vin_sense_adc.channel_id);
            continue;
        }

        float vin_adc_data_v = (vin_adc_data_mv * mv_to_v_multiplier) * adc_gain;
        if (k_msgq_put(&adc_telemetry_msgq, &vin_adc_data_v, K_NO_WAIT)) {
        }

        // Buffer up data for logging before boost. If no space, throw out the oldest entry.
        if (!logging_enabled && k_msgq_num_free_get(&adc_logging_msgq) == 0) {
            float throwaway_data;
            k_msgq_get(&adc_logging_msgq, &throwaway_data, K_NO_WAIT);
        }

        if (k_msgq_put(&adc_logging_msgq, &vin_adc_data_v, K_MSEC(ADC_UPDATE_TIME_MS))) {
        }

        k_msleep(15);
    }
}