#pragma once
#include <array>
#include <cstdint>
#include <f_core/flight/c_phase_controller.h>

/**
 * Flight Events for a fictional flight
 */
enum Events : uint8_t { PadReady, Boost, Coast, Noseover, MainChute, GroundHit, CamerasOff, NumEvents };
inline constexpr std::array<const char *, Events::NumEvents> eventNames = {
    "PadReady", "Boost", "Coast", "Noseover", "MainChute", "GroundHit", "CamerasOff",
};

/**
 * Sources of flight events
 */
enum Sources : uint8_t {
    IMU1,
    Barom1,
    NoseoverLockout,
    Boost2CoastTimer,
    Noseover2MainTimer,
    FullFlightTimer,
    ExtraCameraTimer,
    NumSources
};
inline constexpr std::array<const char *, Sources::NumSources> sourceNames = {
    "IMU 1 (LSM6DSL)",     "Barom 1 (BME280)",  "Noseover Lockout",  "Boost2Coast Timer",
    "Noseover2Main Timer", "Full Flight Timer", "ExtraCamera Timer",
};

inline constexpr std::size_t num_timer_events = 5;
using Controller = CPhaseController<Events, Events::NumEvents, Sources, Sources::NumSources, num_timer_events>;

/**
 * Special events triggered not by sensors but by timers between phases
 */
inline std::array<Controller::TimerEvent, num_timer_events> timer_events = {
    // The engine should burn for around X seconds. don't detect coast unless the engine has been firing for a bit
    // Can be implemented as a lockout or as another way of progressing states if you don't want to do unboost detection
    Controller::TimerEvent{
        .start = Events::Boost,
        .event = Events::Coast,
        .time = K_SECONDS(2),
        .source = Sources::Boost2CoastTimer,
    },
    // We dont want to accidentally detect noseover when still burning or while going really fast.
    // Adds a lockout timer so even if our sensors say we nosed over, don't trust them until we've had time to slowdown
    Controller::TimerEvent{
        .start = Events::Boost,
        .event = Events::Noseover,
        .time = K_SECONDS(5),
        .source = Sources::NoseoverLockout,
    },
    // Rather than deploying chutes at an altitude, deploy them at a certain time after noseover
    Controller::TimerEvent{
        .start = Events::Noseover,
        .event = Events::MainChute,
        .time = K_SECONDS(150),
        .source = Sources::Noseover2MainTimer,
    },
    // We know our entire flight will not last longer than X seconds even if we main at apogee.
    //This stops us from overwriting flight data if we don't detect the end
    Controller::TimerEvent{
        .start = Events::Boost,
        .event = Events::GroundHit,
        .time = K_SECONDS(200),
        .source = Sources::FullFlightTimer,
    },
    // After we hit the ground, keep the cameras going for a while longer so they start a new video file and when we cut the power, no actual flight footage is lost
    Controller::TimerEvent{
        .start = Events::GroundHit,
        .event = Events::CamerasOff,
        .time = K_SECONDS(2),
        .source = Sources::ExtraCameraTimer,
    },
};

/**
 * Functions for merging the different sources of events
 * 
 * silly immediatly invoked lambda because c++ doesnt support designtaed array initializers :(
 * still constexpr tho which is nice
 */
inline constexpr std::array<Controller::DecisionFunc, Events::NumEvents> deciders = [] {
    std::array<Controller::DecisionFunc, Events::NumEvents> arr = {nullptr};
    // Ready to go
    arr[Events::PadReady] = [](Controller::SourceStates states) -> bool {
        return states[Sources::IMU1] && states[Sources::Barom1];
    };

    // Boosting
    arr[Events::Boost] = [](Controller::SourceStates states) -> bool {
        return (states[Sources::IMU1] || states[Sources::Barom1]);
    };

    // Coasting
    arr[Events::Coast] = [](Controller::SourceStates states) -> bool {
        return states[Sources::Boost2CoastTimer] || states[Sources::IMU1];
    };

    // Noseover
    arr[Events::Noseover] = [](Controller::SourceStates states) -> bool {
        return states[Sources::NoseoverLockout] && states[Sources::Barom1];
    };

    // Main
    arr[Events::MainChute] = [](Controller::SourceStates states) -> bool {
        return states[Sources::Barom1] || states[Sources::Noseover2MainTimer];
    };

    // On the ground
    arr[Events::GroundHit] = [](Controller::SourceStates states) -> bool {
        return states[Sources::Barom1] || states[Sources::FullFlightTimer];
    };

    // After finishing stuff up (what grim did to make sure camera SD cards fully saved)
    arr[Events::CamerasOff] = [](Controller::SourceStates states) -> bool { return states[Sources::ExtraCameraTimer]; };

    return arr;
}();
