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

struct Parameters {
    static constexpr uint32_t MAGIC = 2'435'220'000; // the number that louis told me
    uint32_t magic = MAGIC;                          // if equal to MAGIC, a flight has happened
    // measurments that need to be saved
    uint32_t timestampOfBoost = {0};
    float preBoostPressure = {0};
    NTypes::GyroscopeData gyroBias = {0};
    uint32_t bootcount = {0};
    // constants so you don't accidentally misinterpret the data thats there if you've flashed since
    uint32_t lockoutMs = {0};
    uint32_t numFlightPackets = {0};
    uint32_t numPreboostPackets = {0};
    uint32_t numSamplesForGyroBias = {0};
    uint32_t controllerHash = {0}; // TODO: hash of CSV of LUT that ran this flight
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

/**
 * Cancel flight from anywhere at anytime. 
 * Will stop the main mission to let you test or dump data
 * thread safe and cool
 */
void CancelFlight();

/**
 * Check if the flight has been cancelled
 * @return true if the flight has been cancelled. False otherwise
 */
bool IsFlightCancelled();
