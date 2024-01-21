/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telem.h"

#include <launch_core/backplane_defs.h>
#include <launch_core/device_utils.h>
#include <launch_core/net_utils.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL
);

K_QUEUE_DEFINE(ina_processing_queue);
#define STACK_SIZE (2048)

static K_THREAD_STACK_DEFINE(ina_stack, STACK_SIZE);
static struct k_thread ina_thread;

static const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);

static void ina_task(void *, void *, void *) {
    const struct device *sensors[] = {
            DEVICE_DT_GET(DT_ALIAS(inabatt)), // Battery
            DEVICE_DT_GET(DT_ALIAS(ina3v3)), // 3v3
            DEVICE_DT_GET(DT_ALIAS(ina5v0)) // 5v0
    };

    const bool ina_battery_found = !l_check_device(sensors[0]);
    const bool ina_3v3_found = !l_check_device(sensors[1]);
    const bool ina_5v0_found = !l_check_device(sensors[2]);

    const enum sensor_channel ina_channels[] = {
            SENSOR_CHAN_CURRENT,
            SENSOR_CHAN_VOLTAGE,
            SENSOR_CHAN_POWER
    };

    ina_data_t data_battery = {0};
    ina_data_t data_3v3 = {0};
    ina_data_t data_5v0 = {0};

    ina_task_data_t ina_task_data = {0};

    if (!ina_battery_found) {
        LOG_ERR("INA219 battery sensor not found");
    }

    if (!ina_3v3_found) {
        LOG_ERR("INA219 3v3 sensor not found");
    }

    if (!ina_5v0_found) {
        LOG_ERR("INA219 5v0 sensor not found");

        // TODO: Testing and publishing data
        while (true) {
            l_update_sensors(sensors, 3);
            if (ina_battery_found) {
                l_get_sensor_data(sensors[0], 3, ina_channels, (struct sensor_value **) &data_battery);
            }

            if (ina_3v3_found) {
                l_get_sensor_data(sensors[1], 3, ina_channels, (struct sensor_value **) &data_3v3);
            }

            if (ina_5v0_found) {
                l_get_sensor_data(sensors[2], 3, ina_channels, (struct sensor_value **) &data_5v0);
            }


            ina_task_data.current_battery = sensor_value_to_float(&data_battery.current);
            ina_task_data.voltage_battery = sensor_value_to_float(&data_battery.voltage);
            ina_task_data.power_battery = sensor_value_to_float(&data_battery.power);

            ina_task_data.current_3v3 = sensor_value_to_float(&data_3v3.current);
            ina_task_data.voltage_3v3 = sensor_value_to_float(&data_3v3.voltage);
            ina_task_data.power_3v3 = sensor_value_to_float(&data_3v3.power);

            ina_task_data.current_5v0 = sensor_value_to_float(&data_5v0.current);
            ina_task_data.voltage_5v0 = sensor_value_to_float(&data_5v0.voltage);
            ina_task_data.power_5v0 = sensor_value_to_float(&data_5v0.power);

//        k_queue_append(ina_processing_queue, &ina_task_data);
        }
    }
}

static int init(void) {
    char ip[MAX_IP_ADDRESS_STR_LEN];
    int ret = -1;

    k_queue_init(&ina_processing_queue);
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

    k_thread_create(&ina_thread, &ina_stack[0], STACK_SIZE, ina_task, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0,
                    K_NO_WAIT);
    k_thread_start(&ina_thread);

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

