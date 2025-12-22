/*
 * Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <f_core/utils/n_gnss_utils.h>
#include <zephyr/ztest.h>
#include <math.h>

ZTEST(gnss_utils, test_nanodegrees_to_degrees_float) {
    // Test positive latitude conversion
    int64_t nanodegrees = 45123456789LL; // 45.123456789 degrees
    float result = NGnssUtils::NanodegreesToDegreesFloat(nanodegrees);
    zassert_within(result, 45.123456789f, 0.0001f, 
        "NanodegreesToDegreesFloat conversion failed for positive value");

    // Test negative latitude conversion
    nanodegrees = -90000000000LL; // -90.0 degrees
    result = NGnssUtils::NanodegreesToDegreesFloat(nanodegrees);
    zassert_within(result, -90.0f, 0.0001f, 
        "NanodegreesToDegreesFloat conversion failed for negative value");

    // Test zero
    nanodegrees = 0;
    result = NGnssUtils::NanodegreesToDegreesFloat(nanodegrees);
    zassert_within(result, 0.0f, 0.0001f, 
        "NanodegreesToDegreesFloat conversion failed for zero");
}

ZTEST(gnss_utils, test_nanodegrees_to_degrees_double) {
    // Test positive latitude conversion
    int64_t nanodegrees = 45123456789LL; // 45.123456789 degrees
    double result = NGnssUtils::NanodegreesToDegreesDouble(nanodegrees);
    zassert_within(result, 45.123456789, 0.0000001, 
        "NanodegreesToDegreesDouble conversion failed for positive value");

    // Test negative latitude conversion
    nanodegrees = -90000000000LL; // -90.0 degrees
    result = NGnssUtils::NanodegreesToDegreesDouble(nanodegrees);
    zassert_within(result, -90.0, 0.0000001, 
        "NanodegreesToDegreesDouble conversion failed for negative value");

    // Test zero
    nanodegrees = 0;
    result = NGnssUtils::NanodegreesToDegreesDouble(nanodegrees);
    zassert_within(result, 0.0, 0.0000001, 
        "NanodegreesToDegreesDouble conversion failed for zero");
}

ZTEST(gnss_utils, test_millimeters_to_meters_float) {
    // Test positive altitude conversion
    int64_t millimeters = 1500000LL; // 1500 meters
    float result = NGnssUtils::MillimetersToMetersFloat(millimeters);
    zassert_within(result, 1500.0f, 0.001f, 
        "MillimetersToMetersFloat conversion failed for positive value");

    // Test negative altitude conversion
    int64_t negative_mm = -500000LL; // -500 meters
    result = NGnssUtils::MillimetersToMetersFloat(negative_mm);
    zassert_within(result, -500.0f, 0.001f, 
        "MillimetersToMetersFloat conversion failed for negative value");

    // Test zero
    millimeters = 0;
    result = NGnssUtils::MillimetersToMetersFloat(millimeters);
    zassert_within(result, 0.0f, 0.001f, 
        "MillimetersToMetersFloat conversion failed for zero");
}

ZTEST(gnss_utils, test_millimeters_to_meters_double) {
    // Test positive altitude conversion
    int64_t millimeters = 1500000LL; // 1500 meters
    double result = NGnssUtils::MillimetersToMetersDouble(millimeters);
    zassert_within(result, 1500.0, 0.0001, 
        "MillimetersToMetersDouble conversion failed for positive value");

    // Test negative altitude conversion
    int64_t negative_mm = -500000LL; // -500 meters
    result = NGnssUtils::MillimetersToMetersDouble(negative_mm);
    zassert_within(result, -500.0, 0.0001, 
        "MillimetersToMetersDouble conversion failed for negative value");

    // Test zero
    millimeters = 0;
    result = NGnssUtils::MillimetersToMetersDouble(millimeters);
    zassert_within(result, 0.0, 0.0001, 
        "MillimetersToMetersDouble conversion failed for zero");
}

ZTEST_SUITE(gnss_utils, NULL, NULL, NULL, NULL, NULL);
