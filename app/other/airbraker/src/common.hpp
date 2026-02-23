#pragma once
#include "n_autocoder_types.h"

#include <cstddef>
#include <cstdint>

// time for burnout and decellerating under .8 mach after start of boost
constexpr uint32_t LOCKOUT_MS = 0 * 1000;
constexpr uint32_t FLIGHT_TIME_MS = 1 * 60 * 1000;
constexpr uint32_t NUM_FLIGHT_PACKETS = FLIGHT_TIME_MS / 10; // 100 a sec, 1 every 10ms

constexpr size_t NUM_STORED_PREBOOST_PACKETS = 50;
constexpr size_t NUM_SAMPLES_FOR_GYRO_BIAS = 200;
constexpr size_t NUM_SAMPLES_OVER_BOOST_THRESHOLD_REQUIRED = 25;

static_assert(NUM_STORED_PREBOOST_PACKETS > NUM_SAMPLES_OVER_BOOST_THRESHOLD_REQUIRED,
              "Need packets going back in time in order to feed the averager");

struct KalmanState {
    float estAltitude;
    float estVelocity;
    float estAcceleration;
    float estBias;
};

struct Packet {
    uint32_t timestamp;
    float tempRaw;
    float pressureRaw;
    float accelRaw;

    KalmanState kalmanState; // 4 floats

    NTypes::GyroscopeData gyro; 
    float orientationQuat[4];   
    float effort;               
    // if downsizing, add flags byte (has detected boost, has disabled bc out of bounds, )
};

static_assert(sizeof(Packet) == 64, "Check size of packet");