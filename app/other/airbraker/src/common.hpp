#pragma once
#include "n_autocoder_types.h"

#include <cstddef>
#include <cstdint>

// time for burnout and decellerating under .8 mach after start of boost
constexpr uint32_t LOCKOUT_MS = 3 * 1000;
// from boost to ground hit time
constexpr uint32_t FLIGHT_TIME_MS = 3 * 60 * 1000;
// number of packets at 100hz to save for that length
constexpr uint32_t NUM_FLIGHT_PACKETS = FLIGHT_TIME_MS / 10; // 100 a sec, 1 every 10ms

// number of packets stored preboost (in ram, dumped later)
constexpr size_t NUM_STORED_PREBOOST_PACKETS = 50;
// number of gyro samples that contribute to gyro bias calculation
constexpr size_t NUM_SAMPLES_FOR_GYRO_BIAS = 200;
// number of samples that are required to exceed boost threshold before we decide boost is decided
constexpr size_t NUM_SAMPLES_OVER_BOOST_THRESHOLD_REQUIRED = 25;
// thershold to exceed to start counting towards boost detect
constexpr float BOOST_DETECT_THRESHOLD_MS2 = 9.8 * 10;

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

    KalmanState kalmanState;

    NTypes::GyroscopeData gyro;
    float orientationQuat[4];
    float effort;
};

static_assert(sizeof(Packet) == 64, "Check size of packet");



void CancelFlight();
bool IsFlightCancelled();