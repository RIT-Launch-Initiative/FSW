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
static const uint8_t MAX_IP_ADDRESS_STR_LEN = 16;
static const uint8_t BACKPLANE_NETWORK_ID[2] = {192, 168};

static const uint16_t LAUNCH_EVENT_NOTIFICATION_PORT = 9999;

/********** POWER MODULE **********/
static const uint8_t POWER_MODULE_ID = 1;

static const uint16_t POWER_MODULE_BASE_PORT = 11000;

typedef enum {
    INA_DATA = 15,
    ADC_DATA = 50,
} l_power_module_port_offset_t;


/********** RADIO MODULE **********/
static const uint8_t RADIO_MODULE_ID = 2;
static const uint16_t RADIO_MODULE_BASE_PORT = 12000;

typedef enum {
    GNSS_DATA = 20
} l_radio_module_port_offset_t;


/********** SENSOR MODULE **********/
static const uint8_t SENSOR_MODULE_ID = 3;
static const uint16_t SENSOR_MODULE_BASE_PORT = 13000;

typedef enum {
    TEN_HZ_DATA = 10,
    HUNDRED_HZ_DATA = 100,
} l_sensor_module_port_offset_t;

#endif //BACKPLANE_DEFS_H
