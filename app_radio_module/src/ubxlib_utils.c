/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file ubxlib_utils.h
 * @authors Nate Aquino naquino14@outlook.com
 * @brief Ublox Library Utilities for Radio Module
 */

#include "ubxlib_utils.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ubxlib_utils);

// #define MAX0_NODE DT_ALIAS(max0)
// static const struct device *const max0 = DEVICE_DT_GET(MAX0_NODE);

// /// TODO: Fill in the correct values for these
// #define I2C1_NODE DT_ALIAS(i2c1)
// #define MAXM10S_I2C DEVICE_DT_GET(I2C1_NODE)
// #define MAXM10S_SDA_PIN -1
// #define MAXM10S_SCL_PIN -1

// TODO: get rid of gnss_dev_t and use uGnssTransportHandle_t instead
int start_maxm10s(gnss_dev_t* dev) {
    int ret = uDeviceMutexCreate();
    if (ret != 0) {
        LOG_ERR("Failed to create mutex");
        return ret;
    }

//    ret = uPortInit();
//    if (ret != 0) {
//        LOG_ERR("uPortInit() returned %d\n", ret);
//        return ret;
//    }
//
//    ret = uPortI2cInit();
//    if (ret != 0) {
//        LOG_ERR("uPortI2cInit() returned %d\n", ret);
//        return ret;
//    }
//
//    ret = uDeviceInit();
//    if (ret != 0) {
//        LOG_ERR("uDeviceInit() returned %d\n", ret);
//        return ret;
//    }

    ret = uDeviceOpen(NULL, dev->gnssHandle);
    dev->transportHandle = (uGnssTransportHandle_t)NULL;
    if (ret != 0) {
        LOG_ERR("uDeviceOpen() returned %d\n", ret);
        return ret;
    }
    // Print gnss messages to the i2c line
    uGnssSetUbxMessagePrint(dev->gnssHandle, true);

    // Start the GNSS module
    return uGnssPwrOn(dev->gnssHandle);
}

// TODO: update this to work with uDeviceClose
int stop_maxm10s(gnss_dev_t* dev) {
    uint32_t ret = uGnssPwrOff(dev->gnssHandle);
    if (ret != 0)
        return ret;

    // deallocate the GNSS instance
    uGnssDeinit();
    uPortDeinit();
    dev->transportHandle.i2c = -1;
    dev->gnssHandle = NULL;
    return 0;
}
