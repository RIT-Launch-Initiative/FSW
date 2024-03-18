/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef POWER_MODULE_DEFS_H_
#define POWER_MODULE_DEFS_H_
#include <zephyr/drivers/sensor.h>

// Aligned structs for Zephyr queues
typedef struct {
    uint32_t timestamp;
    ina_data_t data_battery;
    ina_data_t data_3v3;
    ina_data_t data_5v0;
    int16_t vin_adc_data_mv;
} power_module_telemetry_t;

// Packed data for sending over the network and storing as compactly as possible
typedef struct __attribute__((packed)) {
    float current_battery;
    float voltage_battery;
    float power_battery;

    float current_3v3;
    float voltage_3v3;
    float power_3v3;

    float current_5v0;
    float voltage_5v0;
    float power_5v0;

    float vin_adc_data_v;
} power_module_telemetry_packed_t;


#endif
