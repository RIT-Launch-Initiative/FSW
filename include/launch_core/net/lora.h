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
int l_lora_configure(const struct device *dev, bool transmit);

/**
 * Transmit a message over LoRa.
 * @param lora_dev - Device to transmit over
 * @param buff - Buffer of data to transmit
 * @param len - Size of the buffer
 * @return Zephyr status code
 */
int l_lora_tx(const struct device *lora_dev, uint8_t *buff, size_t len);

/**
 * Debug callback function for LoRa reception. Should never be called directly
 * @param dev - Device that received the message
 * @param data - Buffer to store the received data
 * @param size - Size of the buffer
 * @param rssi - Received signal strength indicator
 * @param snr - Signal to noise ratio
 */
void l_lora_debug_recv_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi, int8_t snr);

/**
 * Configure LoRa radio devices for transmission or reception.
 * @param dev - Device to configure
 * @param transmit - True if the device should be configured for transmission. False otherwise
 * @return Zephyr status code
 */
int l_lora_set_tx_rx(const struct device *dev, bool transmit);

/**
 * Set the frequency of the LoRa device
 * @param dev - Device to configure
 * @param frequency - Frequency to set the device to
 * @return Zephyr status code
 */
int l_lora_set_frequency(const struct device *dev, uint32_t frequency);

/**
 * Set the bandwidth of the LoRa device
 * @param dev - Device to configure
 * @param bandwidth - Bandwidth to set the device to
 * @return Zephyr status code
 */
int l_lora_set_bandwidth(const struct device *dev, enum lora_signal_bandwidth bandwidth);

/**
 * Set the data rate of the LoRa device
 * @param dev - Device to configure
 * @param data_rate - Data rate to set the device to
 * @return Zephyr status code
 */
int l_lora_set_data_rate(const struct device *dev, enum lora_datarate data_rate);

/**
 * Set the coding rate of the LoRa device
 * @param dev - Device to configure
 * @param coding_rate - Coding rate to set the device to
 * @return
 */
int l_lora_set_coding_rate(const struct device *dev, enum lora_coding_rate coding_rate);

/**
 * Set the preamble length of the LoRa device
 * @param dev - Device to configure
 * @param preamble_len - Preamble length to set the device to
 * @return Zephyr status code
 */
int l_lora_set_preamble_len(const struct device *dev, uint16_t preamble_len);

/**
 * Set the transmission power of the LoRa device
 * @param dev - Device to configure
 * @param tx_power - Transmission power to set the device to
 * @return Zephyr status code
 */
int l_lora_set_tx_power(const struct device *dev, int8_t tx_power);

#endif // L_LORA_UTILS_H_