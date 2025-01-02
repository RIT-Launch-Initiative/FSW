#ifndef F_CORE_FLIGHT_C_PHASE_CONTROLLER_H
#define F_CORE_FLIGHT_C_PHASE_CONTROLLER_H

#include <array>
#include <cstdint>
#include <cstdio>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>

int flight_log_init(const char *flight_log_file_name, fs_file_t *fp);
int flight_log_source_event(bool enabled, fs_file_t *fp, const char *source, const char *event);
int flight_log_event_confirmed(bool enabled, fs_file_t *fp, const char *event, bool currentState);
int flight_log_print(bool enabled, fs_file_t *fp, const char *str);
int flight_log_close(bool enabled, fs_file_t *fp);

/**
 * Phase Controller
 * Handles the detection of states for a rocket flight.
 * *Events* are things that happen during flight that could affect behavior of the system (Boost, Noseover, Landing, etc)
 * *Sources* are different mechanism the flight computer has of detecting events (IMU, Barometer, GPS, Timer, etc)
 * @tparam EventID an enum type identifying Events. Any value of type EventID should not exceed num_events
 * @tparam num_events the number of unique events that will be passed to any member function. num_events > any value of type EventID
 * @tparam SourceID an enum type identifying Sources. Any value of type SourceID should not exceed num_sources
 * @tparam num_sources the number of unique sources that will be passed to any member function. num_sources > any value of type SourceID
 * @tparam num_timers the number of timer-triggered events that will be used
 */
template <typename EventID, std::size_t num_events, typename SourceID, std::size_t num_sources, std::size_t num_timers>
class CPhaseController {
  public:
    static_assert(num_events <= 32, "Current implementation is limited to 32 events");
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
    using SourceStates = std::array<bool, num_sources>;

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
    CPhaseController(const std::array<const char *, num_sources> &sourceNames,
                     const std::array<const char *, num_events> &eventNames,
                     const std::array<TimerEvent, num_timers> &timerEvents,
                     const std::array<DecisionFunc, num_events> &deciders, const char *flightLogFileName = nullptr)
        : sourceNames(sourceNames), eventNames(eventNames), deciders(deciders), flightLogFileName(flightLogFileName) {

        for (std::size_t i = 0; i < num_timers; i++) {
            timerUserdata[i] = InternalTimerEvent{.controller = this, .event = timerEvents[i]};
            k_timer_init(&timers[i], timer_expiry_cb, NULL);
            k_timer_user_data_set(&timers[i], (void *) &timerUserdata[i]);
        }

        k_event_init(&osEvents);
        k_event_clear(&osEvents, 0xFFFFFFFF);

        flight_log_init(flightLogFileName, &flightLogFile);
    }

    /**
     * Write an arbitrary line to the flight log file (if enabled)
     * A newline will be written to the file after every message
     * @param str the null terminated string to write to the log file. 
     * @return the return value of fs_write. If >=0, the number of characters written. If <0, an error code.
     */
    int WriteToLog(const char *str) { return flight_log_print(HasFlightLog(), &flightLogFile, str); }

    /**
     * Tell the controller that a specific source thinks an event has happened
     * If this submission of an event causes the decider to return true, the controller will start reporting that the event has occured
     * @param source the SourceID of the source that thinks an event has happened
     * @param event the event that the source thinks happened
     */
    void SubmitEvent(SourceID source, EventID event) {
        uint32_t event_bit = (1 << (uint32_t) event);
        sourceStates[event][source] = true;

        // If that event submission caused the event to fully trigger, send message
        if (deciders[event](sourceStates[event])) {
            bool last_state = eventStates[event];
            if (!last_state) {
                // dispatch event
                eventStates[event] = true;
                k_event_post(&osEvents, event_bit);

                // start any necessary timers
                for (std::size_t i = 0; i < num_timers; i++) {
                    if (timerUserdata[i].event.start == event) {
                        k_timer_start(&timers[i], timerUserdata[i].event.time, K_NO_WAIT);
                    }
                }
            }
            flight_log_source_event(HasFlightLog(), &flightLogFile, sourceNames[source], eventNames[event]);
            flight_log_event_confirmed(HasFlightLog(), &flightLogFile, eventNames[event], last_state);
        } else {
            flight_log_source_event(HasFlightLog(), &flightLogFile, sourceNames[source], eventNames[event]);
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
        uint32_t event_bit = (1 << (uint32_t) event);
        uint32_t event_that_happened = k_event_wait(&osEvents, event_bit, false, timeout);
        return event_that_happened != 0;
    }
    /**
     * Check if the phase controller has a flight log
     * @return true if the phase controller will write its events to a file. false if not
     */
    bool HasFlightLog() { return flightLogFileName != nullptr; }

    /**
     * Closes the flight log and saves to disk
     * The contents of the flight log may  not be saved until this is called
     * This is called in the destructor as well. 
     * If you can verify that the destructor will be called, you do not need to call this.
     */
    void CloseFlightLog() { flight_log_close(HasFlightLog(), &flightLogFile); }

    ~CPhaseController() { CloseFlightLog(); }

  private:
    /** 
     * Type for holding information to fire timer events correctly. 
    */
    struct InternalTimerEvent {
        CPhaseController *controller; //< 'this' pointer
        TimerEvent event;             //< information about the event
    };

    /**
     * callback function for timer events. 'this' is stored in the InternalTimerEvent data
     */
    constexpr static auto timer_expiry_cb = [](struct k_timer *timer) {
        void *data = k_timer_user_data_get(timer);
        InternalTimerEvent ievt = *static_cast<InternalTimerEvent *>(data);
        ievt.controller->SubmitEvent(ievt.event.source, ievt.event.event);
    };

    // Current State of the system

    /// state of events per source
    std::array<SourceStates, num_events> sourceStates = {false};
    /// the state of events that have been agreed to have happened based on deciders and per-source states
    std::array<bool, num_events> eventStates = {false};

    // Timer handling
    std::array<struct k_timer, num_timers> timers = {0};
    std::array<InternalTimerEvent, num_timers> timerUserdata = {0};

    // OS events for handling synchronization
    k_event osEvents;

    // consts for logging and deciding. These will not change after construction
    const std::array<const char *, num_sources> &sourceNames;
    const std::array<const char *, num_events> &eventNames;

    const std::array<DecisionFunc, num_events> &deciders;

    // Flight log
    const char *flightLogFileName;
    fs_file_t flightLogFile;
};

#endif