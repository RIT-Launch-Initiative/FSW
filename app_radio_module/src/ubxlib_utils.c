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

/// TODO: Fill in the correct values for these
#define MAXM10S_I2C -1
#define MAXM10S_SDA_PIN -1
#define MAXM10S_SCL_PIN -1

int start_maxm10s(gnss_dev_t* dev) {
    // Initialize the APIs
    uint32_t ret = uPortInit();
    if (ret != 0)
        return ret;
    printk("uPortInit() returned %d\n", ret);

    ret = uGnssInit();
    if (ret != 0)
        return ret;

    // open a UART port to the MAX-M10S GNSS module
    // I am going to leave this as I2C for now, SPI and UART
    // also exist. TODO: Macro values should be filled in based
    // devicetree shenanigans
    dev->transportHandle.i2c = uPortI2cOpen(MAXM10S_I2C,
                                            MAXM10S_SDA_PIN,
                                            MAXM10S_SCL_PIN,
                                            true);
    if (dev->transportHandle.i2c < 0)
        return dev->transportHandle.i2c;

    // Add a GNSS instance and telling it to spit data out
    // on the i2c transport handle.
    ret = uGnssAdd(U_GNSS_MODULE_TYPE_M10,  // our module is a MAX-M10S
                   U_GNSS_TRANSPORT_I2C,
                   dev->transportHandle,
                   U_CFG_APP_PIN_GNSS_ENABLE_POWER,
                   false,
                   dev->gnssHandle);
    if (ret != 0)
        return ret;

    // Print gnss messages to the i2c line
    uGnssSetUbxMessagePrint(dev->gnssHandle, true);

    // Start the GNSS module
    return uGnssPwrOn(dev->gnssHandle);
}

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
