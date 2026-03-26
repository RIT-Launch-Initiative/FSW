/*
 * Copyright (c) 2023 Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
/**
 * Definitions for all backplane related constants
 */

#include <stdint.h>

/********** GENERAL **********/
static const uint16_t LAUNCH_EVENT_NOTIFICATION_PORT = 9999;

typedef enum {
    L_BOOST_DETECTED = 'b',
    L_APOGEE_DETECTED = 'a',
    L_DROGUE_DEPLOYED = 'd',
    L_MAIN_DEPLOYED = 'm',
    L_LANDING_DETECTED = 'l',
} l_event_notification_t;

/********** POWER MODULE **********/
#define POWER_MODULE_ID 1
#define POWER_MODULE_BASE_PORT 11000

typedef enum {
    POWER_MODULE_INA_DATA_PORT = 15,
    POWER_MODULE_ADC_DATA_PORT = 50,
} l_power_module_port_offset_t;


/********** RADIO MODULE **********/
#define RADIO_MODULE_ID 2
#define RADIO_MODULE_BASE_PORT 12000

typedef enum {
    RADIO_MODULE_GNSS_DATA_PORT = 1
} l_radio_module_port_offset_t;


/********** SENSOR MODULE **********/
#define SENSOR_MODULE_ID 3
#define SENSOR_MODULE_BASE_PORT 13000

typedef enum {
    SENSOR_MODULE_TEN_HZ_DATA_PORT = 10,
    SENSOR_MODULE_HUNDRED_HZ_DATA_PORT = 100,
} l_sensor_module_port_offset_t;


