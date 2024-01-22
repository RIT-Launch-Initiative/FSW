/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "power_module_defs.h"

#include <launch_core/backplane_defs.h>
#include <launch_core/device_utils.h>
#include <launch_core/net_utils.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define SENSOR_READ_STACK_SIZE (512)
#define QUEUE_PROCESSING_STACK_SIZE (1024)
#define INA219_UPDATE_TIME_MS (67)

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

static struct k_msgq ina_processing_queue;
static uint8_t ina_processing_queue_buffer[CONFIG_INA219_QUEUE_SIZE * sizeof(ina_task_data_t)];

static K_THREAD_STACK_DEFINE(ina_read_stack, SENSOR_READ_STACK_SIZE);
static struct k_thread ina_read_thread;

static K_THREAD_STACK_DEFINE(ina_processing_stack, QUEUE_PROCESSING_STACK_SIZE);
static struct k_thread ina_processing_thread;

static const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);

static void ina_task(void *, void *, void *) {
    const struct device *sensors[] = {
            DEVICE_DT_GET(DT_ALIAS(inabatt)), // Battery
            DEVICE_DT_GET(DT_ALIAS(ina3v3)), // 3v3
            DEVICE_DT_GET(DT_ALIAS(ina5v0)) // 5v0
    };

    const bool ina_device_found[] = {
            l_check_device(sensors[0]) == 0,
            l_check_device(sensors[1]) == 0,
            l_check_device(sensors[2]) == 0
    };

    const enum sensor_channel ina_channels[] = {
            SENSOR_CHAN_CURRENT,
            SENSOR_CHAN_VOLTAGE,
            SENSOR_CHAN_POWER
    };

    ina_task_data_t ina_task_data = {0};

    if (!ina_device_found[0]) {
        LOG_ERR("INA219 battery sensor not found");
    }

    if (!ina_device_found[1]) {
        LOG_ERR("INA219 3v3 sensor not found");
    }

    if (!ina_device_found[2]) {
        LOG_ERR("INA219 5v0 sensor not found");
    }

    while (true) {
        l_update_sensors_safe(sensors, 3, ina_device_found);
        ina_task_data.timestamp = k_uptime_get_32();

        struct sensor_value current;
        struct sensor_value voltage;
        struct sensor_value power;
        struct sensor_value *sensor_values[] = {
                &current,
                &voltage,
                &power
        };

        if (likely(ina_device_found[0])) {
            l_get_sensor_data(sensors[0], 3, ina_channels, sensor_values);
            ina_task_data.data_battery.current = sensor_value_to_float(&current);
            ina_task_data.data_battery.voltage = sensor_value_to_float(&voltage);
            ina_task_data.data_battery.power = sensor_value_to_float(&power);
        }

        if (likely(ina_device_found[1])) {
            l_get_sensor_data(sensors[1], 3, ina_channels, sensor_values);
            ina_task_data.data_3v3.current = sensor_value_to_float(&current);
            ina_task_data.data_3v3.voltage = sensor_value_to_float(&voltage);
            ina_task_data.data_3v3.power = sensor_value_to_float(&power);
        }

        if (likely(ina_device_found[2])) {
            l_get_sensor_data(sensors[2], 3, ina_channels, sensor_values);
            ina_task_data.data_5v0.current = sensor_value_to_float(&current);
            ina_task_data.data_5v0.voltage = sensor_value_to_float(&voltage);
            ina_task_data.data_5v0.power = sensor_value_to_float(&power);
        }

        if (k_msgq_put(&ina_processing_queue, &ina_task_data, K_NO_WAIT)) {
            LOG_ERR("Failed to put data into INA219 processing queue");
        }

        // Wait some time for sensor to get new values (15 Hz -> 66.67 ms)
        uint32_t time_to_wait = INA219_UPDATE_TIME_MS - (k_uptime_get_32() - ina_task_data.timestamp);
        if (time_to_wait > 0) {
            k_sleep(K_MSEC(time_to_wait));
        }
    }
}

static void ina_queue_processing_task(void *, void *, void *) {
    ina_task_data_t ina_task_data = {0};
    ina_packed_data_t ina_packed_data = {0};

    while (true) {
        if (k_msgq_get(&ina_processing_queue, &ina_task_data, K_FOREVER)) {
            LOG_ERR("Failed to get data from INA219 processing queue");
            continue;
        }

        ina_packed_data.current_battery = ina_task_data.data_battery.current;
        ina_packed_data.voltage_battery = ina_task_data.data_battery.voltage;
        ina_packed_data.power_battery = ina_task_data.data_battery.power;

        ina_packed_data.current_3v3 = ina_task_data.data_3v3.current;
        ina_packed_data.voltage_3v3 = ina_task_data.data_3v3.voltage;
        ina_packed_data.power_3v3 = ina_task_data.data_3v3.power;

        ina_packed_data.current_5v0 = ina_task_data.data_5v0.current;
        ina_packed_data.voltage_5v0 = ina_task_data.data_5v0.voltage;
        ina_packed_data.power_5v0 = ina_task_data.data_5v0.power;

        // TODO: write to flash when data logging library is ready
        l_send_udp_broadcast((uint8_t *) &ina_packed_data, sizeof(ina_packed_data_t), POWER_MODULE_BASE_PORT + POWER_MODULE_INA_DATA_PORT);
    }
}

static int init(void) {
    char ip[MAX_IP_ADDRESS_STR_LEN];
    int ret = -1;

    k_msgq_init(&ina_processing_queue, ina_processing_queue_buffer, sizeof(ina_task_data_t), CONFIG_INA219_QUEUE_SIZE);
    if (0 > l_create_ip_str_default_net_id(ip, POWER_MODULE_ID, 1)) {
        LOG_ERR("Failed to create IP address string: %d", ret);
        return -1;
    }

    if (!l_check_device(wiznet)) {
        ret = l_init_udp_net_stack(ip);
        if (ret != 0) {
            LOG_ERR("Failed to initialize network stack");
            return ret;
        }
    } else {
        LOG_ERR("Failed to get network device");
        return ret;
    }

    // TODO: Play with these values on rev 2 where we can do more profiling
    k_thread_create(&ina_read_thread, &ina_read_stack[0], SENSOR_READ_STACK_SIZE, ina_task, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
    k_thread_start(&ina_read_thread);

    k_thread_create(&ina_processing_thread, &ina_processing_stack[0], QUEUE_PROCESSING_STACK_SIZE, ina_queue_processing_task, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0,
                    K_NO_WAIT);
    k_thread_start(&ina_processing_thread);

    return 0;
}


int main(void) {
    if (init()) {
        return -1;
    }

    while (true) {
        k_sleep(K_MSEC(100));
    }

    return 0;

}