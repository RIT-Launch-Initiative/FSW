/*
 * Copyright (c) 2024 Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "f_core/utils/debouncer.hpp"

#include <stdio.h>
#include <string.h>
#include <zephyr/ztest.h>

ZTEST(debouncer, test_working_good) {
    using Timestamp = uint32_t;
    using Value = float;
    Value timeOver = 100; //ms
    Value valOver = 10.0; //meters or something

    CDebouncer<ThresholdDirection::Over, Timestamp, Value> db(timeOver, valOver);

    zassert_equal(db.Passed(), false, "Have no data. shouldn't be passing");
    // first sample
    db.Feed(0, 5.0);
    zassert_equal(db.Passed(), false, "Have only one point. shouldn't be passing");

    // second sample
    db.Feed(100, 15.0);
    zassert_equal(db.Passed(), false, "2 points. 1 under, 1 over. shouldn't be passing");
    // second sample
    db.Feed(150, 15.0);
    zassert_equal(db.Passed(), false, "2 points. both over but less than time. shouldn't be passing");

    db.Feed(201, 15.0);
    zassert_equal(db.Passed(), true, "2 points. both over with enough time. should be passing");
}
ZTEST(debouncer, test_working_zig_zag) {
    using Timestamp = uint32_t;
    using Value = float;
    Value timeOver = 100; //ms
    Value valOver = 10.0; //meters or something

    CDebouncer<ThresholdDirection::Over, Timestamp, Value> db(timeOver, valOver);

    zassert_equal(db.Passed(), false, "Have no data. shouldn't be passing");
    // first sample
    db.Feed(0, 5.0);
    zassert_equal(db.Passed(), false, "Have only one point. shouldn't be passing");

    // second sample
    db.Feed(100, 15.0);
    zassert_equal(db.Passed(), false, "2 points. 1 under, 1 over. shouldn't be passing");
    // second sample
    db.Feed(150, 5.0);
    zassert_equal(db.Passed(), false, "fell back under. shouldn't be passing");

    db.Feed(100, 15.0);
    zassert_equal(db.Passed(), false, "2 points. 1 under, 1 over. shouldn't be passing");
}
ZTEST(debouncer, test_simulated_flight) {
    using Timestamp = float;
    using Value = float;
    Value timeOver = 0.25;
    Value valOver = 4.0; //meters or something
    auto flight = [](Timestamp t) -> Value { return (-2 * t * t + 6 * t); };

    CDebouncer<ThresholdDirection::Over, Timestamp, Value> db(timeOver, valOver);

    for (float t = 0; t < 2; t += 0.001) {
        Value v = flight(t);
        db.Feed(t, v);
        printf("%.3f\t%.3f: %d\n", t, v, int(db.Passed()));

        if (t < 1.0 + timeOver) {
            zassert_false(db.Passed(), "havent achieved height + timeOver seconds: t=%f, h=%f", t, v);
        }
        float epsilon = 0.001;
        if (t > 1.0 + timeOver + epsilon) {
            zassert_true(db.Passed(), "have achieved height for timeOver seconds: t=%f, h=%f", t, v);
        }
    }
}

ZTEST_SUITE(debouncer, NULL, NULL, NULL, NULL, NULL);
