#ifndef F_CORE_FLIGHT_C_PHASE_CONTROLLER_H
#define F_CORE_FLIGHT_C_PHASE_CONTROLLER_H

#include <array>
#include <cstdint>
#include <cstdio>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>

#include <f_core/os/flight_log.hpp>

/**
 * Phase Controller
 * Handles the detection of states for a rocket flight.
 * *Events* are things that happen during flight that could affect behavior of the system (Boost, Noseover, Landing, etc)
 * *Sources* are different mechanism the flight computer has of detecting events (IMU, Barometer, GPS, Timer, etc)
 * @tparam EventID an enum type identifying Events. Any value of type EventID should not exceed num_events
 * @tparam numEvents the number of unique events that will be passed to any member function. num_events > any value of type EventID
 * @tparam SourceID an enum type identifying Sources. Any value of type SourceID should not exceed num_sources
 * @tparam numSources the number of unique sources that will be passed to any member function. num_sources > any value of type SourceID
 * @tparam numTimers the number of timer-triggered events that will be used
 */
template <typename EventID, std::size_t numEvents, typename SourceID, std::size_t numSources, std::size_t numTimers>
class CPhaseController {
  public:
    static_assert(numEvents <= 32, "Current implementation is limited to 32 events");
    static_assert(std::is_enum_v<EventID>, "EventIDs must be enums");
    static_assert(std::is_enum_v<SourceID>, "SourceID must be enums");

    /**
     * Description of a timer-triggered event
     */
    struct TimerEvent {
        EventID start;    //< The event to start the timer at
        EventID event;    //< The event to send when the timer expires
        k_timeout_t time; //< The time after the start event is triggered to expire
        SourceID source;  //< Named source that the event will be sent from
    };

    /**
     * States of all the sources for a certain event
     * Example:
     * Source: IMU1  Barom  GNSS
     * State:  true  true   false
     * Decision functions take this and come to a conclusion about whether or
     * not the event has passed the threshold for actually happening
     */
    using SourceStates = std::array<bool, numSources>;

    /**
     * Decide if an event has actually happened based on whether or not the possible sources believe it has
     * Decision functions should be intterupt-safe
     * quick operations that don't do IO or sleep or anything spooky
     */
    using DecisionFunc = bool (*)(SourceStates sourceStates);

    /**
     * Construct a Phase Controller
     * @param sourceNames an array of human readable names for sources indexed by SourceID
     * @param eventNames an array of human readable names for events indexed by 
     * @param timerEvents an array of desciptions of all timer triggered events. See TimerEvent
     * @param deciders an array of decision functions for deciding if an event has really occured based on the state of the sources
     * @param flightLogFileName a file name 
     * The lifetime of all these paramters should exceed the lifetime of an instance of CPhaseController
     */
    CPhaseController(const std::array<const char *, numSources> &sourceNames,
                     const std::array<const char *, numEvents> &eventNames,
                     const std::array<TimerEvent, numTimers> &timerEvents,
                     const std::array<DecisionFunc, numEvents> &deciders, CFlightLog *flightLog)
        : sourceNames(sourceNames), eventNames(eventNames), deciders(deciders), flightLog(flightLog) {

        for (std::size_t i = 0; i < numTimers; i++) {
            timerUserdata[i] = InternalTimerEvent{.controller = this, .event = timerEvents[i]};
            k_timer_init(&timers[i], timer_expiry_cb, NULL);
            k_timer_user_data_set(&timers[i], (void *) &timerUserdata[i]);
        }

        k_event_init(&osEvents);
        k_event_clear(&osEvents, 0xFFFFFFFF);
        if (flightLog != nullptr) {
            flightLog->Write("CPhaseController initialized");
        }
    }
    ~CPhaseController() {
        for (std::size_t i = 0; i < numTimers; i++) {
            k_timer_stop(&timers[i]);
        }
    }

    /**
     * Gets the flight log that this controller is reporting to
     * @return pointer to the active flight log. null if no flight log was passed in
     */
    CFlightLog *flightLog() { return flightLog; }

    /**
     * Log a message like "1234ms: Boost from IMU1"
     * @param event the event that occured 'Boost'
     * @param source the source that triggered this event 'IMU1'
     * @return 0 if successfully written to the flight log. Otherwise, the filesystem error from writing.
     */
    int LogSourceEvent(EventID event, SourceID source) {
        if (flightLog != nullptr) {
            constexpr size_t bufferSize = 64;
            char stringBuffer[bufferSize] = {0};
            int bytesWritten = snprintf(stringBuffer, bufferSize, "%-10s from %s", eventNames[event], sourceNames[source]);
            return flightLog->Write(stringBuffer, bytesWritten);
        }
        return 0;
    }

    /**
     * Logs a message like "Boost confirmed" or "Noseover confirmed but already happened. Not Dispatching"
     * @param event the event that was confirmed
     * @param currentState the state of that even as gotten from HasEventOccured(event) *before* this confirmation. 
     * @return 0 if successfully written to the flight log. Otherwise, the filesystem error from writing.
     */
    int LogEventConfirmed(EventID event, bool currentState) {
        if (flightLog != nullptr) {
            constexpr size_t buf_size = 64;
            char string_buf[buf_size] = {0};
            int num_wrote = snprintf(string_buf, buf_size, "%-10s confirmed%s", eventNames[event],
                                     currentState ? " but already happened. Not dispatching" : "");
            return flightLog->Write(string_buf, num_wrote);
        }
        return 0;
    }

    /**
     * Tell the controller that a specific source thinks an event has happened
     * If this submission of an event causes the decider to return true, the controller will start reporting that the event has occured
     * @param source the SourceID of the source that thinks an event has happened
     * @param event the event that the source thinks happened
     */
    void SubmitEvent(SourceID source, EventID event) {
        uint32_t eventBit = (1 << (uint32_t) event);
        sourceStates[event][source] = true;

        // If that event submission caused the event to fully trigger, send message
        if (deciders[event](sourceStates[event])) {
            bool lastState = eventStates[event];
            if (!lastState) {
                // dispatch event
                eventStates[event] = true;
                k_event_post(&osEvents, eventBit);

                // start any necessary timers
                for (std::size_t i = 0; i < numTimers; i++) {
                    if (timerUserdata[i].event.start == event) {
                        k_timer_start(&timers[i], timerUserdata[i].event.time, K_NO_WAIT);
                    }
                }
            }
            LogSourceEvent(event, source);
            LogEventConfirmed(event, lastState);
        } else {
            LogSourceEvent(event, source);
        }
    }

    /**
     * Check if a given event has occured yet. 
     * Checks the state of the event after the decision function NOT the per-source event states
     * @return true if the requested event has occured
     */
    bool HasEventOccured(EventID event) { return eventStates[event]; }

    /**
     * Wait for an event to occur (or timeout first)
     * @param event the event to wait for
     * @param timeout 
     * @return True if the event occured. False if the timeout occured.
     */
    bool WaitUntilEvent(EventID event, k_timeout_t timeout = K_FOREVER) {
        uint32_t eventBit = (1 << (uint32_t) event);
        uint32_t eventThatHappened = k_event_wait(&osEvents, eventBit, false, timeout);
        return eventThatHappened != 0;
    }

  private:
    /** 
     * Type for holding information to fire timer events correctly. 
    */
    struct InternalTimerEvent {
        CPhaseController *controller; //< 'this' pointer
        TimerEvent event;             //< information about the event
    };

    /**
     * Callback function for timer events. 
     * 'this' is stored in the InternalTimerEvent data
     * @param timer the timer containing that is calling this callback. Contains user data
     */
    constexpr static auto timer_expiry_cb = [](struct k_timer *timer) {
        void *data = k_timer_user_data_get(timer);
        InternalTimerEvent eventInfo = *static_cast<InternalTimerEvent *>(data);
        eventInfo.controller->SubmitEvent(eventInfo.event.source, eventInfo.event.event);
    };

    // Current State of the system

    /// state of events per source
    std::array<SourceStates, numEvents> sourceStates = {false};
    /// the state of events that have been agreed to have happened based on deciders and per-source states
    std::array<bool, numEvents> eventStates = {false};

    // Timer handling
    std::array<struct k_timer, numTimers> timers = {0};
    std::array<InternalTimerEvent, numTimers> timerUserdata = {0};

    // OS events for handling synchronization
    k_event osEvents;

    // consts for logging and deciding. These will not change after construction
    const std::array<const char *, numSources> &sourceNames;
    const std::array<const char *, numEvents> &eventNames;

    const std::array<DecisionFunc, numEvents> &deciders;

    // Flight Log or nullptr if no logging is requested.
    CFlightLog *flightLog;
};

#endif