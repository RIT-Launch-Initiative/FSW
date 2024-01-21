/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telem.h"

#include <stdint.h>

#include <launch_core/device_utils.h>
#include <launch_core/net_utils.h>

#include <zephyr/kernel.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>

#define STACK_SIZE (2048)

LOG_MODULE_REGISTER(telem);

//static K_THREAD_STACK_DEFINE(adc_stack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(ina_stack, STACK_SIZE);
static struct k_thread ina_thread;

static power_module_data_t power_module_data = {0};

static void ina_task(void *, void *, void *) {
    // TODO: Maybe alias each sensor in the DTS
    const struct device *sensors[] = {
            DEVICE_DT_GET(DT_ALIAS(inabatt)), // Battery
            DEVICE_DT_GET(DT_ALIAS(ina3v3)), // 3v3
            DEVICE_DT_GET(DT_ALIAS(ina5v0)) // 5v0
    };

    const enum sensor_channel ina_channels[] = {
            SENSOR_CHAN_CURRENT,
            SENSOR_CHAN_VOLTAGE,
            SENSOR_CHAN_POWER
    };

    ina_data_t data_battery = {0};
    ina_data_t data_3v3 = {0};
    ina_data_t data_5v0 = {0};


    // TODO: Testing and publishing data
    while (true) {
        l_update_sensors(sensors, 3);
        l_get_sensor_data(sensors[0], 3, ina_channels, (struct sensor_value **) &data_battery);
        l_get_sensor_data(sensors[1], 3, ina_channels, (struct sensor_value **) &data_3v3);
        l_get_sensor_data(sensors[2], 3, ina_channels, (struct sensor_value **) &data_5v0);
    }
}

// static void adc_task(void *unused0, void *unused1, void *unused2) {
//     uint16_t buff;
//     
//     struct adc_sequence adc_seq = {
//         .buffer = &buff,
//         .buffer_size = sizeof(buff)
//     };
//
//     static const struct adc_channel_cfg vin_volt_sens_channel = ADC_CHANNEL_CFG_DT(adc1);
//     // if (!adc_is_ready_dt()) {
//     //     LOG_ERR("ADC device is not ready\n");
//     //     return;
//     // }
//
//     // if (!adc_channel_setup_dt()) {
//     //     LOG_ERR("ADC channel setup failed\n");
//     //     return;
//     // }
//
//
//     while (1) {
//         int32_t tmp = 0;
//         // if (!adc_read(, &adc_seq)) {
//         //     LOG_ERR("ADC read failed\n");
//         //     continue;
//         // }
//         //
//         // if (adc_raw_to_millivolts_dt(, &tmp)) {
//             power_module_data.vin_voltage_sense = tmp;
//         k_msleep(1000);
//         // }
//     };
// }

void init_telem_tasks() {
    k_thread_create(&ina_thread, &ina_stack[0], STACK_SIZE, ina_task, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
    k_thread_start(&ina_thread);
}

void convert_and_send() {
    static power_module_packet_t packet = {0};
    static uint8_t flip_flop = 0;

    packet.current_battery = sensor_value_to_float(&power_module_data.ina_battery.current);
    packet.voltage_battery = sensor_value_to_float(&power_module_data.ina_battery.voltage);
    packet.power_battery = sensor_value_to_float(&power_module_data.ina_battery.power);

    packet.current_3v3 = sensor_value_to_float(&power_module_data.ina_3v3.current);
    packet.voltage_3v3 = sensor_value_to_float(&power_module_data.ina_3v3.voltage);
    packet.power_3v3 = sensor_value_to_float(&power_module_data.ina_3v3.power);

    packet.current_5v0 = sensor_value_to_float(&power_module_data.ina_5v0.current);
    packet.voltage_5v0 = sensor_value_to_float(&power_module_data.ina_5v0.voltage);
    packet.power_5v0 = sensor_value_to_float(&power_module_data.ina_5v0.power);
    packet.vin_voltage_sense = flip_flop ? 0xDEAD : 0xBEEF;
    flip_flop ^= 0b1;


    l_send_udp_broadcast((const uint8_t *) &packet, sizeof(power_module_packet_t), 9000);
}
