/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKPLANE_DEFS_H
#define BACKPLANE_DEFS_H

/********** GENERAL **********/
static const uint8_t BACKPLANE_NETWORK_ID[2] = {192, 168};
static const uint8_t LAUNCH_EVENT_NOTIFICATION_PORT = 9999;

/********** POWER MODULE **********/
static const uint8_t POWER_MODULE_ID = 1;

static const uint8_t POWER_MODULE_BASE_PORT = 11000;

typedef enum : uint8_t {
    INA_DATA = 15,
    ADC_DATA = 50,
} POWER_MODULE_PORT_OFFSET_T;


/********** RADIO MODULE **********/
static const uint8_t RADIO_MODULE_ID = 2;
static const uint8_t RADIO_MODULE_BASE_PORT = 12000;

typedef enum : uint8_t {
    GNSS_DATA = 20
} RADIO_MODULE_PORT_OFFSET_T;


/********** SENSOR MODULE **********/
static const uint8_t SENSOR_MODULE_ID = 3;
static const uint8_t SENSOR_MODULE_BASE_PORT = 13000;

typedef enum : uint8_t {
    TEN_HZ_DATA = 10,
    HUNDRED_HZ_DATA = 100,
} SENSOR_MODULE_PORT_OFFSET_T;

#endif //BACKPLANE_DEFS_H
