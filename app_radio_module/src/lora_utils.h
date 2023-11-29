/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LORA_UTILS_H_
#define LORA_UTILS_H_

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>

int init_sx1276(const struct device *const dev);

int lora_configure(const struct device *const dev, bool transmit);

int lora_tx(const struct device *const lora_dev, uint8_t *buff, size_t len);

void lora_debug_recv_cb(const struct device *const dev, uint8_t *data, uint16_t size, int16_t rssi, int8_t snr);

#endif
