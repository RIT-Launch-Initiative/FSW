/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LORA_UTILS_H_
#define LORA_UTILS_H_

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>

typedef struct __attribute__((__packed__)) {
    uint16_t port;
    float pressure_ms5;
    float temperature_ms5;

    float pressure_bmp3;
    float temperature_bmp3;

    float accel_x;
    float accel_y;
    float accel_z;

    float magn_x;
    float magn_y;
    float magn_z;

    float gyro_x;
    float gyro_y;
    float gyro_z;

    float temperature_tmp;
} FAKE_SENSOR_DATA_T;

/**
 * @brief Initialize the SX1278 Radio Module
 * @details This device is the one that uses the LoRa protocol
 * @param dev The device to initialize
 * @return int 0 if successful, negative errno code if failure
 */
int init_sx1276(const struct device *const dev);

/**
 * @brief Configure the LoRa protocol
 *
 * @param dev the LoRa device
 * @param transmit true if we are transmitting, false if we are receiving
 * @return int 0 if successful, negative errno code if failure
 */
int lora_configure(const struct device *const dev, bool transmit);

/**
 * @brief Transmit LoRa data
 *
 * @param lora_dev the LoRa device
 * @param buff The data to transmit
 * @param len The size of the data to transmit
 * @return int 0 if successful, negative errno code if failure
 */
int lora_tx(const struct device *const lora_dev, uint8_t *buff, size_t len);

/**
 * @brief AARON PLS COMMENT IDFK WHAT THIS DOES
 *
 * @param dev The LoRa device prolly
 * @param data The data to receive?
 * @param size The size of the data to recieve
 * @param rssi
 * @param snr
 */
void lora_debug_recv_cb(const struct device *const dev, uint8_t *data, uint16_t size, int16_t rssi, int8_t snr);

#endif
