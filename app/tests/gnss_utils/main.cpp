/*
 * Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <f_core/utils/n_gnss_utils.h>
#include <zephyr/ztest.h>

ZTEST(gnss_utils, test_nanodegrees_to_degrees_float) {
    zassert_within(NGnssUtils::NanodegreesToDegreesFloat(45123456789LL), 45.123456789f, 0.0001f);
    zassert_within(NGnssUtils::NanodegreesToDegreesFloat(-90000000000LL), -90.0f, 0.0001f);
    zassert_within(NGnssUtils::NanodegreesToDegreesFloat(0), 0.0f, 0.0001f);
}

ZTEST(gnss_utils, test_nanodegrees_to_degrees_double) {
    zassert_within(NGnssUtils::NanodegreesToDegreesDouble(45123456789LL), 45.123456789, 0.0000001);
    zassert_within(NGnssUtils::NanodegreesToDegreesDouble(-90000000000LL), -90.0, 0.0000001);
    zassert_within(NGnssUtils::NanodegreesToDegreesDouble(0), 0.0, 0.0000001);
}

ZTEST(gnss_utils, test_millimeters_to_meters_float) {
    zassert_within(NGnssUtils::MillimetersToMetersFloat(1500000LL), 1500.0f, 0.001f);
    zassert_within(NGnssUtils::MillimetersToMetersFloat(-500000LL), -500.0f, 0.001f);
    zassert_within(NGnssUtils::MillimetersToMetersFloat(0), 0.0f, 0.001f);
}

ZTEST(gnss_utils, test_millimeters_to_meters_double) {
    zassert_within(NGnssUtils::MillimetersToMetersDouble(1500000LL), 1500.0, 0.0001);
    zassert_within(NGnssUtils::MillimetersToMetersDouble(-500000LL), -500.0, 0.0001);
    zassert_within(NGnssUtils::MillimetersToMetersDouble(0), 0.0, 0.0001);
}

ZTEST_SUITE(gnss_utils, NULL, NULL, NULL, NULL, NULL);
