/*
 * Copyright (c) 2024 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Utility functions for dealing with Zephyr ADC devices
 */

#ifndef L_ADC_UTILS_H
#define L_ADC_UTILS_H

#include <zephyr/drivers/adc.h>

/**
 * Initialize an ADC channel for reading
 * @param channel - ADC Channel to initialize
 * @param sequence - ADC Sequence to initialize
 * @return Zephyr status code
 */
int l_init_adc_channel(const struct adc_dt_spec *const channel, struct adc_sequence *const sequence);

/**
 * Initialize multiple ADC channels for reading
 * @param channels - Pointer to an array of ADC channels to initialize
 * @param sequences - Pointer to an array of ADC sequences to initialize
 * @param num_channels - Number of channels to initialize
 * @return Zephyr status code
 */
int l_init_adc_channels(const struct adc_dt_spec *const channels, struct adc_sequence *const sequences, const int num_channels);

/**
 * Read the value of an ADC channel in millivolts synchronously
 * @param channel - ADC Channel to read
 * @param sequence - ADC Sequence to read
 * @param val - Pointer to store the value in
 * @return Zephyr status code
 */
int l_read_adc_mv(const struct adc_dt_spec *const channel, struct adc_sequence *const sequence, int32_t *val);

/**
 * Read the value of an ADC channel in millivolts asynchronously
 * @param channel - ADC Channel to read
 * @param sequence - ADC Sequence to read
 * @param val - Pointer to store the value in
 * @return Zephyr status code
 */
int l_async_read_adc_mv(const struct adc_dt_spec *const channel, struct adc_sequence *const sequence, int32_t *val);

#endif // L_ADC_UTILS_H