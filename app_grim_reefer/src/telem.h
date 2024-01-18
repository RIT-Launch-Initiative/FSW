/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/sensor.h>

#ifndef POWER_MODULE_TELEM_H_
#define POWER_MODULE_TELEM_H_


typedef struct {
    struct sensor_value current;
    struct sensor_value voltage;
    struct sensor_value power;
    int32_t vin_voltage_sense;
} ina_data_t;

typedef struct {
    ina_data_t ina_battery;
    ina_data_t ina_3v3;
    ina_data_t ina_5v0;
    int16_t vin_voltage_sense;
} power_module_data_t;

typedef struct __attribute__((__packed__)) {
    float current_battery;
    float voltage_battery;
    float power_battery;
    
    float current_3v3;
    float voltage_3v3;
    float power_3v3;

    float current_5v0;
    float voltage_5v0;
    float power_5v0;
    
    int16_t vin_voltage_sense;
} power_module_packet_t;


void init_telem_tasks();

void convert_and_send();
#endif
