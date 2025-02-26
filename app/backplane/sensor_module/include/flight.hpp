#ifndef SENSOR_MOD_FLIGHT_H_
#define SENSOR_MOD_FLIGHT_H_
#include <array>
#include <cstdint>
#include <f_core/flight/c_phase_controller.h>

// Boost
static constexpr double boost_threshold_m_s2 = 5 * 9.8; //m/s^2
static constexpr uint32_t boost_time_thresshold = 250;  //ms

// Noseover
static constexpr double noseover_velocity_thresshold = 5; //ft/s
static constexpr uint32_t noseover_time_thresshold = 250; //ms

// Ground
static constexpr double ground_velocity_thresshold = 10;  //ft/s
static constexpr uint32_t ground_time_thresshold = 10000; //ms (10s)

enum Events : uint8_t { Boost, NoseoverLockout, Noseover, GroundHit, CamerasOff, NumEvents };
inline constexpr std::array<const char *, Events::NumEvents> eventNames = {
    "Boost", "NoseoverLockout", "Noseover", "GroundHit", "CamerasOff",
};

/**
 * Sources of flight events
 */
enum Sources : uint8_t {
    LowGImu,
    HighGImu,
    BaromBMP,
    BaromMS5611,
    NoseoverLockout,
    FullFlightTimer,
    VideoOffTimer,
    NumSources
};
inline constexpr std::array<const char *, Sources::NumSources> sourceNames = {
    "LowGIMU (LSM6DSL)", "HighGImu (ADXL)",   "BaromBMP",     "BaromMS5611",
    "Noseover Lockout",  "Full Flight Timer", "VideoOffTimer"};

inline constexpr std::size_t num_timer_events = 3;
using SensorModulePhaseController =
    CPhaseController<Events, Events::NumEvents, Sources, Sources::NumSources, num_timer_events>;

/**
 * Special events triggered not by sensors but by timers between phases
 */
inline std::array<SensorModulePhaseController::TimerEvent, num_timer_events> timer_events = {
    // We dont want to accidentally detect noseover when still burning or while going really fast.
    // Adds a lockout timer so even if our sensors say we nosed over, don't trust them until we've had time to slowdown
    SensorModulePhaseController::TimerEvent{
        .start = Events::Boost,
        .event = Events::Noseover,
        .time = K_SECONDS(15),
        .source = Sources::NoseoverLockout,
    },
    // We know our entire flight will not last longer than X seconds even if we main at apogee.
    //This stops us from overwriting flight data if we don't detect the end
    SensorModulePhaseController::TimerEvent{
        .start = Events::Boost,
        .event = Events::GroundHit,
        .time = K_SECONDS(350),
        .source = Sources::FullFlightTimer,
    },
    // After we hit the ground, keep the cameras going for a while longer
    SensorModulePhaseController::TimerEvent{
        .start = Events::GroundHit,
        .event = Events::CamerasOff,
        .time = K_SECONDS(100),
        .source = Sources::VideoOffTimer,
    },
};

/**
 * Functions for merging the different sources of events
 * 
 * silly immediately invoked lambda because c++ doesn't support designated array initializers :(
 * still constexpr tho which is nice
 */
inline constexpr std::array<SensorModulePhaseController::DecisionFunc, Events::NumEvents> deciders = [] {
    std::array<SensorModulePhaseController::DecisionFunc, Events::NumEvents> arr = {nullptr};

    // Boosting (when one of our sensors says go)
    arr[Events::Boost] = [](SensorModulePhaseController::SourceStates states) -> bool {
        return (states[Sources::LowGImu] || states[Sources::HighGImu] || states[Sources::BaromBMP] ||
                states[Sources::BaromMS5611]);
    };

    // NoseoverLockout
    arr[Events::NoseoverLockout] = [](SensorModulePhaseController::SourceStates states) -> bool {
        return states[Sources::NoseoverLockout];
    };

    // Noseover
    arr[Events::Noseover] = [](SensorModulePhaseController::SourceStates states) -> bool {
        return states[Sources::BaromBMP] && states[Sources::BaromMS5611];
    };

    // On the ground
    arr[Events::GroundHit] = [](SensorModulePhaseController::SourceStates states) -> bool {
        return states[Sources::BaromBMP] || states[Sources::BaromMS5611] || states[Sources::FullFlightTimer];
    };

    // After finishing stuff
    arr[Events::CamerasOff] = [](SensorModulePhaseController::SourceStates states) -> bool {
        return states[Sources::VideoOffTimer];
    };

    return arr;
}();

#endif