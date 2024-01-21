/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/sensor.h>

#ifndef POWER_MODULE_DEFS_H_
#define POWER_MODULE_DEFS_H_


typedef struct __attribute__((__packed__)) {
    float current;
    float voltage;
    float power;
} ina_data_t;

typedef struct __attribute__((__packed__)) {
    ina_data_t data_battery;
    ina_data_t data_3v3;
    ina_data_t data_5v0;
} ina_task_data_t;

#endif
