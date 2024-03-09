/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKPLANE_DEFS_H
#define BACKPLANE_DEFS_H

/**
 * Definitions for all backplane related constants
 */

#include <stdint.h>

/********** GENERAL **********/

static const uint16_t LAUNCH_EVENT_NOTIFICATION_PORT = 9999;

/********** POWER MODULE **********/
static const uint8_t POWER_MODULE_ID = 1;

static const uint16_t POWER_MODULE_BASE_PORT = 11000;

typedef enum {
    POWER_MODULE_INA_DATA_PORT = 15,
    POWER_MODULE_ADC_DATA_PORT = 50,
} l_power_module_port_offset_t;


/********** RADIO MODULE **********/
static const uint8_t RADIO_MODULE_ID = 2;
static const uint16_t RADIO_MODULE_BASE_PORT = 12000;

typedef enum {
    RADIO_MODULE_GNSS_DATA_PORT = 20
} l_radio_module_port_offset_t;


/********** SENSOR MODULE **********/
static const uint8_t SENSOR_MODULE_ID = 3;
static const uint16_t SENSOR_MODULE_BASE_PORT = 13000;

typedef enum {
    SENSOR_MODULE_TEN_HZ_DATA_PORT = 10,
    SENSOR_MODULE_HUNDRED_HZ_DATA_PORT = 100,
} l_sensor_module_port_offset_t;

#endif //BACKPLANE_DEFS_H
