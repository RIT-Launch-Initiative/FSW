/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
* Utility functions for dealing with Zephyr's LoRa library
*/

#ifndef L_LORA_UTILS_H_
#define L_LORA_UTILS_H_

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>

/**
 * Utility functions for dealing with Zephyr's LoRa library
 */

/**
 * Configure LoRa radio devices for transmission or reception.
 * @param dev - Device to configure
 * @param transmit - True if the device should be configured for transmission. False otherwise
 * @return Zephyr status code
 */
int l_lora_configure(const struct device *const dev, bool transmit);

/**
 * Transmit a message over LoRa.
 * @param lora_dev - Device to transmit over
 * @param buff - Buffer of data to transmit
 * @param len - Size of the buffer
 * @return Zephyr status code
 */
int l_lora_tx(const struct device *const lora_dev, uint8_t *buff, size_t len);

/**
 * Debug callback function for LoRa reception. Should never be called directly
 * @param dev - Device that received the message
 * @param data - Buffer to store the received data
 * @param size - Size of the buffer
 * @param rssi - Received signal strength indicator
 * @param snr - Signal to noise ratio
 */
void l_lora_debug_recv_cb(const struct device *const dev, uint8_t *data, uint16_t size, int16_t rssi, int8_t snr);


#endif // L_LORA_UTILS_H_