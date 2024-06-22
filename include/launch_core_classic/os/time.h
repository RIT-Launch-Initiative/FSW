/*
 * Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Utility functions for keeping track of the time of day
 */


#ifndef L_TIME_H
#define L_TIME_H

#include <stdint.h>

/**
 * Set the time of day in milliseconds
 * @param time_of_day_ms - Time of day in milliseconds
 */
void l_init_time(uint32_t time_of_day_ms);

/**
 * Get the time of day in milliseconds
 * @param current_uptime_ms - Current uptime in milliseconds
 * @return Time of day in milliseconds, or 0 if the time of day has not been set
 */
uint32_t l_get_time_of_day_ms(uint32_t current_uptime_ms);

#endif // L_TIME_H
