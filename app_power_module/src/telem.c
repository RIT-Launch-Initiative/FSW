/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telem.h"
#include <launch_core/net_utils.h>

#include <stdint.h>

#include <zephyr/kernel.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>

#define STACK_SIZE (2048)

LOG_MODULE_REGISTER(telem, CONFIG_APP_LOG_LEVEL);

//static K_THREAD_STACK_DEFINE(adc_stack, STACK_SIZE);
static K_THREAD_STACK_ARRAY_DEFINE(stacks, 4, STACK_SIZE);

static power_module_data_t power_module_data = {0};
static struct k_thread threads[4] = {0};

static void ina_task(void *p_id, void *unused1, void *unused2) {
    const struct device *dev;
    ina_data_t *ina_data;

    switch (POINTER_TO_INT(p_id)) {
        case 0:
            dev = DEVICE_DT_GET(DT_INST(0, ti_ina219));
            ina_data = &power_module_data.ina_battery;
            break;
        case 1:
            dev = DEVICE_DT_GET(DT_INST(1, ti_ina219));
            ina_data = &power_module_data.ina_3v3;
            break;
        case 2:
            dev = DEVICE_DT_GET(DT_INST(2, ti_ina219));
            ina_data = &power_module_data.ina_5v0;
            break;
        default:
            return;
    }


    while (true) {
        sensor_sample_fetch(dev);
        sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE, &ina_data->voltage);
        sensor_channel_get(dev, SENSOR_CHAN_POWER, &ina_data->power);
        sensor_channel_get(dev, SENSOR_CHAN_CURRENT, &ina_data->current);
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
    // TODO: Maybe make this one thread that does all the INA stuff
    for (int i = 0; i < 3; i++) {
        k_thread_create(&threads[i], &stacks[i][0], STACK_SIZE,
                        ina_task, INT_TO_POINTER(i), NULL, NULL,
                        K_PRIO_PREEMPT(10), 0, K_NO_WAIT);

        k_thread_start(&threads[i]);
    }
}

void convert_and_send() {
    static power_module_packet_t packet = {0};
    static uint8_t flip_flop = 0;

    packet.current_battery =  sensor_value_to_float(&power_module_data.ina_battery.current);
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
