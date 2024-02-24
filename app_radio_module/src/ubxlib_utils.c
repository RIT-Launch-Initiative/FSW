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
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(ubxlib_utils);

// #define MAX0_NODE DT_ALIAS(max0)
// static const struct device *const max0 = DEVICE_DT_GET(MAX0_NODE);
static const struct gpio_dt_spec ubx_rst = GPIO_DT_SPEC_GET(DT_ALIAS(ubx_rst), gpios);


static void l_gnss_callback(uDeviceHandle_t gnssHandle,
                               int32_t errorCode,
                               int32_t latitudeX1e7,
                               int32_t longitudeX1e7,
                               int32_t altitudeMillimetres,
                               int32_t radiusMillimetres,
                               int32_t speedMillimetresPerSecond,
                               int32_t svs,
                               int64_t timeUtc) {
    // TODO: Have another function set a callback that this function will redirect to.
    // This callback will encapsulate all data in these arguments into a struct
    // Also, probably LOG data here and add a KConfig option to enable/disable

}


// TODO: get rid of gnss_dev_t and use uGnssTransportHandle_t instead
int init_maxm10s(gnss_dev_t *dev) {
    LOG_INF("Resetting MAX-M10S GNSS Module\n");
    gpio_pin_set_dt(&ubx_rst, GPIO_ACTIVE_LOW);
    k_msleep(500);
    gpio_pin_set_dt(&ubx_rst, GPIO_ACTIVE_HIGH);
    k_msleep(2000);

    int ret = uPortInit();
    if (ret != 0) {
        LOG_ERR("uPortInit() returned %d\n", ret);
        return ret;
    }

    ret = uPortUartInit();
    if (ret != 0) {
        LOG_ERR("uPortUartInit() returned %d\n", ret);
        return ret;
    }

    ret = uDeviceInit();
    if (ret != 0) {
        LOG_ERR("uPortDeviceInit() returned %d\n", ret);
        return ret;
    }

    ret = uDeviceOpen(NULL, &dev->gnssHandle);
    if (ret != 0) {
        LOG_ERR("uDeviceOpen() returned %d\n", ret);
        return ret;
    }

    return 0;
}
//    dev->transportHandle = (uGnssTransportHandle_t) NULL;
//    // Print gnss messages to the i2c line
//
    // Start the GNSS module
//    uDeviceHandle_t dev_handle = NULL;
//    MyContext_t context = {0};
//    uDeviceSetUserContext(dev_handle, (void *) &context);
//    ret = uGnssPosGetStreamedStart(dev_handle,
//                                   U_GNSS_POS_STREAMED_PERIOD_DEFAULT_MS,
//                                   l_gnss_callback);

// TODO: update this to work with uDeviceClose
int stop_maxm10s(gnss_dev_t *dev) {
    uint32_t ret = uGnssPwrOff(dev->gnssHandle);
    if (ret != 0)
        return ret;

    // deallocate the GNSS instance
    uGnssDeinit();
    uPortDeinit();
    dev->transportHandle.uart = -1;
    dev->gnssHandle = NULL;
    return 0;
}
