#ifndef FREAK_FLIGHT_H
#define FREAK_FLIGHT_H
#include "f_core/flight/c_phase_controller.h"

#include <array>

// Boost
// IMU
static constexpr float imuBoostThresholdMPerS2 = 5 * 9.8;  // m/s^2
static constexpr uint32_t imuBoostTimeThreshold = 250;     // millis

// Barometer - sustained velocity of 100 m/s over 1 second
static constexpr float baromBoostThresholdMPerS = 100;    // m/s
static constexpr uint32_t baromBoostTimeThreshold = 1000; // millis

enum Events {
    PadReady,
    Unlocked,
    Boost,
    Coast,
    GroundHit,
    InitialInflation,
    NumEvents,
};

inline constexpr std::array<const char *, Events::NumEvents> eventNames = {
    "PadReady", "Unlocked", "Boost", "Coast", "InitialInflation", "GroundHit",
};
enum Sources {
    // Flight Controlling Sources
    LSM6DSL,
    BMP390,
    BoostTimer,
    GroundHitTimer,
    InflationSubsys,
    NumSources,
};
inline constexpr std::array<const char *, Sources::NumSources> sourceNames = {
    "LSM6DSL", "BMP390", "BoostTimer", "GroundHitTimer", "InflationSubsys",
};

inline constexpr size_t numTimerEvents = 2;
using FreakFlightController = CPhaseController<Events, Events::NumEvents, Sources, Sources::NumSources, 2>;

inline std::array<FreakFlightController::TimerEvent, numTimerEvents> timerEvents = {
    // We only need to measure a lot of data super fast during boost
    // After that, we can chill out until we hit the ground
    FreakFlightController::TimerEvent{
        .start = Events::Boost,
        .event = Events::Coast,
        .time = K_SECONDS(3),
        .source = Sources::BoostTimer,
    },
    FreakFlightController::TimerEvent{
        .start = Events::Boost,
        .event = Events::GroundHit,
        .time = K_SECONDS(15), // 410
        .source = Sources::GroundHitTimer,
    },
};

inline constexpr std::array<FreakFlightController::DecisionFunc, Events::NumEvents> decisionFuncs = [] {
    std::array<FreakFlightController::DecisionFunc, Events::NumEvents> arr = {nullptr};

    arr[Events::PadReady] = [](FreakFlightController::SourceStates states) -> bool {
        return states[Sources::BMP390] && states[Sources::LSM6DSL];
    };

    arr[Events::Boost] = [](FreakFlightController::SourceStates states) -> bool {
        return (states[Sources::LSM6DSL] || states[Sources::BMP390]);
    };

    arr[Events::Coast] = [](FreakFlightController::SourceStates states) -> bool { return states[Sources::BoostTimer]; };

    arr[Events::GroundHit] = [](FreakFlightController::SourceStates states) -> bool {
        return states[Sources::GroundHitTimer];
    };
    arr[Events::InitialInflation] = [](FreakFlightController::SourceStates states) -> bool {
        return states[Sources::InflationSubsys];
    };
    return arr;
}();

#endif