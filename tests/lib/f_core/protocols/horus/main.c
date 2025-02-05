/*
 * Copyright (c) 2024 Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "f_core/protocols/horus/horus.h"

#include <stdio.h>
#include <string.h>
#include <zephyr/ztest.h>

bool horus_packet_equals(const struct horus_packet_v2 *lhs, const struct horus_packet_v2 *rhs){
    bool good = true;
    good &= lhs->payload_id == rhs->payload_id;
    good &= lhs->counter == rhs->counter;
    good &= lhs->hours == rhs->hours;
    good &= lhs->minutes == rhs->minutes;
    good &= lhs->seconds == rhs->seconds;
    good &= lhs->latitude == rhs->latitude;
    good &= lhs->longitude == rhs->longitude;
    good &= lhs->altitude == rhs->altitude;
    good &= lhs->speed == rhs->speed;
    good &= lhs->sats == rhs->sats;
    good &= lhs->temp == rhs->temp;
    good &= lhs->battery_voltage == rhs->battery_voltage;
    good &= lhs->speed == rhs->speed;

    for (size_t i = 0; i < sizeof(lhs->custom_data); i++){    
        good &= lhs->custom_data[i] == rhs->custom_data[i];
    }
    
    good &= lhs->checksum== rhs->checksum;

    return good;
}

ZTEST(horus, test_roundtrip0s) {
    struct horus_packet_v2 packet = {0};
    struct horus_packet_v2 outpacket = {0};
    horus_packet_v2_encoded_buffer_t buf = {0};

    horusv2_encode(&packet, &buf);
    horusv2_decode(&buf, &packet);
        
    zassert_true(horusv2_checksum_verify(&packet), "in packet should have good checksum from encoding");
    zassert_true(horusv2_checksum_verify(&outpacket), "out packet should have good checksum from decoding");
    zassert_true(horus_packet_equals(&packet, &outpacket), "packet should be the same going in and coming out");
}

ZTEST_SUITE(horus, NULL, NULL, NULL, NULL, NULL);