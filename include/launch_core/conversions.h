/*
 * Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MATH_H
#include <math.h>
#endif // _MATH_H

/* Constants */

/* Temperature */
#define CELSIUS_TO_KELVIN 273.15f

/* Pressure */
#define SEA_LEVEL_PRESSURE_BAR 1.01325f

/* Magic */
#define HYPSOMETRIC_FORMULA_DENOMINATOR 0.0065f
#define PRESSURE_MAGIC_EXPONENT 0.190226f

#ifndef CONVERSIONS_H
#define CONVERSIONS_H

/**
 * @brief Calculate the altitude from the pressure and temperature
 * @param pressure The pressure in bar
 * @param temperature The temperature in degrees Celsius
 * @return The altitude in meters
*/
inline float l_alt_fr_press_temp(float pressure, float temperature);

#endif //CONVERSIONS_H