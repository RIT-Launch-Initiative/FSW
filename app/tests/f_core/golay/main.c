/*
 * Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <f_core/radio/protocols/horus/golay.h>
#include <zephyr/ztest.h>

ZTEST(golay, test_encode_decode_no_errors) {
    // Test basic encode-decode cycle with no errors
    int data = 0xABC; // 12-bit data
    int encoded = golay23_encode(data);
    int decoded = golay23_decode(encoded);
    
    zassert_equal(decoded, data, 
        "Decoded data should match original data when no errors introduced");
}

ZTEST(golay, test_encode_multiple_values) {
    // Test encoding of various data values
    int test_values[] = {0x000, 0x001, 0x555, 0xAAA, 0xFFF};
    
    for (int i = 0; i < 5; i++) {
        int data = test_values[i];
        int encoded = golay23_encode(data);
        int decoded = golay23_decode(encoded);
        
        zassert_equal(decoded, data, 
            "Decoded data should match original for value 0x%03x", data);
    }
}

ZTEST(golay, test_single_bit_error_correction) {
    // Test that single bit errors can be corrected
    int data = 0xABC;
    int encoded = golay23_encode(data);
    
    // Introduce a single bit error at position 5
    int corrupted = encoded ^ (1 << 5);
    int decoded = golay23_decode(corrupted);
    
    zassert_equal(decoded, data, 
        "Single bit error should be correctable");
}

ZTEST(golay, test_two_bit_error_correction) {
    // Test that two bit errors can be corrected
    int data = 0x555;
    int encoded = golay23_encode(data);
    
    // Introduce two bit errors at positions 3 and 10
    int corrupted = encoded ^ (1 << 3) ^ (1 << 10);
    int decoded = golay23_decode(corrupted);
    
    zassert_equal(decoded, data, 
        "Two bit errors should be correctable");
}

ZTEST(golay, test_three_bit_error_correction) {
    // Test that three bit errors can be corrected
    int data = 0xAAA;
    int encoded = golay23_encode(data);
    
    // Introduce three bit errors at positions 2, 8, and 15
    int corrupted = encoded ^ (1 << 2) ^ (1 << 8) ^ (1 << 15);
    int decoded = golay23_decode(corrupted);
    
    zassert_equal(decoded, data, 
        "Three bit errors should be correctable");
}

ZTEST(golay, test_error_counting) {
    // Test error counting functionality
    int data = 0x123;
    int encoded = golay23_encode(data);
    
    // Introduce two bit errors
    int corrupted = encoded ^ (1 << 4) ^ (1 << 12);
    int corrected = golay23_decode(corrupted);
    int error_count = golay23_count_errors(corrupted, corrected);
    
    zassert_equal(error_count, 2, 
        "Error count should be 2 for two bit errors");
}

ZTEST(golay, test_zero_data) {
    // Test encoding and decoding of zero
    int data = 0x000;
    int encoded = golay23_encode(data);
    int decoded = golay23_decode(encoded);
    
    zassert_equal(decoded, data, 
        "Zero should encode and decode correctly");
}

ZTEST(golay, test_all_ones_data) {
    // Test encoding and decoding of all ones (12 bits)
    int data = 0xFFF;
    int encoded = golay23_encode(data);
    int decoded = golay23_decode(encoded);
    
    zassert_equal(decoded, data, 
        "All ones (0xFFF) should encode and decode correctly");
}

ZTEST_SUITE(golay, NULL, NULL, NULL, NULL, NULL);
