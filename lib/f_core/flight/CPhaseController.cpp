#include "f_core/flight/CPhaseController.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flight_controller);

void flight_log_init() { LOG_INF("Beginning Flight Log"); }
void flight_log_source_event(const char *source, const char *event) { LOG_INF("%-10s from %s", event, source); }
void flight_log_event_confirmed(const char *event, bool current_state) {
    LOG_INF("%-10s confirmed%s", event, current_state ? " but already happened. Not dispatching" : "");
}