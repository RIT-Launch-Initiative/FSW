/*
 * Copyright (c) 2024 Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "f_core/util/debouncer.hpp"

#include <stdio.h>
#include <string.h>
#include <zephyr/ztest.h>

ZTEST(debouncer, test_working_good) {
    using Timestamp = uint32_t;
    using Value = float;
    Value timeOver = 100; //ms
    Value valOver = 10.0; //meters or something

    CDebuouncer<ThresholdDirection::Over, Timestamp, Value> db(timeOver, valOver);

    zassert_equal(db.passed(), false, "Have no data. shouldn't be passing");
    // first sample
    db.feed(0, 5.0);
    zassert_equal(db.passed(), false, "Have only one point. shouldn't be passing");

    // second sample
    db.feed(100, 15.0);
    zassert_equal(db.passed(), false, "2 points. 1 under, 1 over. shouldn't be passing");
    // second sample
    db.feed(150, 15.0);
    zassert_equal(db.passed(), false, "2 points. both over but less than time. shouldn't be passing");

    db.feed(201, 15.0);
    zassert_equal(db.passed(), true, "2 points. both over with enough time. should be passing");
}
ZTEST(debouncer, test_working_zig_zag) {
    using Timestamp = uint32_t;
    using Value = float;
    Value timeOver = 100; //ms
    Value valOver = 10.0; //meters or something

    CDebuouncer<ThresholdDirection::Over, Timestamp, Value> db(timeOver, valOver);

    zassert_equal(db.passed(), false, "Have no data. shouldn't be passing");
    // first sample
    db.feed(0, 5.0);
    zassert_equal(db.passed(), false, "Have only one point. shouldn't be passing");

    // second sample
    db.feed(100, 15.0);
    zassert_equal(db.passed(), false, "2 points. 1 under, 1 over. shouldn't be passing");
    // second sample
    db.feed(150, 5.0);
    zassert_equal(db.passed(), false, "fell back under. shouldn't be passing");

    db.feed(100, 15.0);
    zassert_equal(db.passed(), false, "2 points. 1 under, 1 over. shouldn't be passing");
}
ZTEST(debouncer, test_simulated_flight) {
    using Timestamp = float;
    using Value = float;
    Value timeOver = 0.25;
    Value valOver = 4.0; //meters or something
    auto flight = [](Timestamp t) -> Value { return (-2 * t * t + 6 * t); };

    CDebuouncer<ThresholdDirection::Over, Timestamp, Value> db(timeOver, valOver);

    for (float t = 0; t < 2; t += 0.001) {
        Value v = flight(t);
        db.feed(t, v);
        printf("%.3f\t%.3f: %d\n", t, v, int(db.passed()));

        if (t < 1.0 + timeOver) {
            zassert_false(db.passed(), "havent achieved height + timeOver seconds: t=%f, h=%f", t, v);
        }
        float epsilon = 0.001;
        if (t > 1.0 + timeOver + epsilon) {
            zassert_true(db.passed(), "have achieved height for timeOver seconds: t=%f, h=%f", t, v);
        }
    }
}

ZTEST_SUITE(debouncer, NULL, NULL, NULL, NULL, NULL);