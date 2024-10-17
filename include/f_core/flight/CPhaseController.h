#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
#include <zephyr/kernel.h>

void flight_log_init();
void flight_log_source_event(const char *source, const char *event);
void flight_log_event_confirmed(const char *event, bool current_state);

template <typename EventID, std::size_t num_events, typename SourceID, std::size_t num_sources, std::size_t num_timers>
class PhaseController {

  public:
    struct TimerEvent {
        EventID start;
        EventID event;
        k_timeout_t time;
        SourceID source;
    };

    // States of all the sources for a certain event
    // Example:
    // Source: IMU1  Barom  GNSS
    // State:  true  true   false
    // Decision functions take this and come to a conclusion about whether or
    // not the event is actually achieved
    using SourceStates = std::array<bool, num_sources>;

    // Decision functions should be intterupt-safe
    // quick operations that don't do IO or sleep or anything spooky
    using DecisionFunc = bool (*)(SourceStates source_states);

    void SubmitEvent(SourceID source, EventID event) {
        source_states[event][source] = true;
        flight_log_source_event(sourceNames[source], eventNames[event]);

        if (deciders[event](source_states[event])) {
            bool state = event_states[event];
            flight_log_event_confirmed(eventNames[event], state);
            if (!state) {
                // dispatch event
                event_states[event] = true;
                // start any necessary timers
                for (std::size_t i = 0; i < num_timers; i++) {
                    if (timer_userdata[i].event.start == event) {
                        k_timer_start(&timers[i], timer_userdata[i].event.time, K_NO_WAIT);
                    }
                }
            }
        }
    }

    PhaseController(const std::array<const char *, num_sources> &sourceNames,
                    const std::array<const char *, num_events> &eventNames,
                    const std::array<TimerEvent, num_timers> &timerEvents,
                    const std::array<DecisionFunc, num_events> &deciders)
        : sourceNames(sourceNames), eventNames(eventNames), deciders(deciders) {
        for (std::size_t i = 0; i < num_timers; i++) {
            timer_userdata[i] = InternalTimerEvent{.controller = this, .event = timerEvents[i]};
            k_timer_init(&timers[i], timer_expiry_cb, NULL);
            k_timer_user_data_set(&timers[i], (void *) &timer_userdata[i]);
        }
    }

    // Dealing with state
    bool HasEventOccured(EventID event) { return event_states[event]; }
    void WaitUntilEvent(EventID event) {
        while (event_states[event] == false) {
            k_msleep(1);
        }
    }

  private:
    struct InternalTimerEvent {
        PhaseController *controller;
        TimerEvent event;
    };
    constexpr static auto timer_expiry_cb = [](struct k_timer *timer) {
        void *data = k_timer_user_data_get(timer);
        InternalTimerEvent ievt = *(InternalTimerEvent *) data;
        ievt.controller->SubmitEvent(ievt.event.source, ievt.event.event);
    };

    // Current State of the system
    std::array<SourceStates, num_events> source_states = {false};
    std::array<bool, num_events> event_states = {false};
    std::array<struct k_timer, num_timers> timers = {0};
    std::array<InternalTimerEvent, num_timers> timer_userdata = {0};

    // consts for logging and deciding. These will not change after construction
    const std::array<const char *, num_sources> &sourceNames;
    const std::array<const char *, num_events> &eventNames;

    const std::array<DecisionFunc, num_events> &deciders;
};