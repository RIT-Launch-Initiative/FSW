#include "openrocket_sensors.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(openrocket, CONFIG_OPENROCKET_LOG_LEVEL);

extern struct or_data_t* or_packets;
extern struct or_event_occurance_t* or_events;

extern const unsigned int or_events_size;
extern const unsigned int or_packets_size;

const char* event_to_str(enum or_event_t e);

int or_data_interpolator() {
    for (int i = 0; i < or_events_size; i++) {
        struct or_event_occurance_t ev = or_events[i];
        LOG_INF("Ev: %s at %f", event_to_str(ev.event), (double) ev.time_s);
    }
    LOG_INF("%d events, %d packets", or_events_size, or_packets_size);
    return or_events_size;
}

const char* event_to_str(enum or_event_t e) {
    static const char* names[] = {
        [OR_EVENT_IGNITION] = "IGNITION",     [OR_EVENT_LAUNCH] = "LAUNCH",
        [OR_EVENT_LIFTOFF] = "LIFTOFF",       [OR_EVENT_LAUNCHROD] = "LAUNCHROD",
        [OR_EVENT_BURNOUT] = "BURNOUT",       [OR_EVENT_EJECTION_CHARGE] = "EJECTION_CHARGE",
        [OR_EVENT_APOGEE] = "APOGEE",         [OR_EVENT_RECOVERY_DEVICE_DEPLOYMENT] = "RECOVERY_DEVICE_DEPLOYMENT",
        [OR_EVENT_GROUND_HIT] = "GROUND_HIT", [OR_EVENT_SIMULATION_END] = "SIMULATION_END",

    };
    return names[e];
}
