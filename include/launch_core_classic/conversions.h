/*
 * Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONVERSIONS_H

#ifndef _MATH_H
#include <math.h>
#endif // _MATH_H

/**
 * @brief Calculate the altitude from the pressure and temperature
 * @param pressure_kpa The pressure in kPa
 * @param temperature_c The temperature in degrees Celsius
 * @return The altitude in meters
*/
double l_altitude_conversion(double pressure_kpa, double temperature_c);

