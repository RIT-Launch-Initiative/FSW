#include "openrocket_sensors.h"

#include <assert.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(openrocket, CONFIG_OPENROCKET_LOG_LEVEL);

// Forward Declarations
static struct or_data_t pad_packet;
static struct or_data_t landed_packet;
static int init_openrocket(void);
SYS_INIT_NAMED(init_openrocket, init_openrocket, POST_KERNEL, 0);

extern const struct or_data_t* const or_packets;
extern const unsigned int or_packets_size;

// Event Log forward declarations
#ifdef CONFIG_OPENROCKET_EVENT_LOG
extern const struct or_event_occurance_t* const or_events;
extern const unsigned int or_events_size;

static const char* event_to_str(enum or_event_t e);

static void or_event_thread_handler(void);
K_THREAD_DEFINE(or_event_thread, 1024, or_event_thread_handler, NULL, NULL, NULL, 0, 0,
                CONFIG_OPENROCKET_MS_BEFORE_LAUNCH);
#endif

int sensor_value_from_or_scalar(struct sensor_value* val, or_scalar_t inp) {
#ifdef CONFIG_OPENROCKET_SCALAR_TYPE_DOUBLE
    return sensor_value_from_double(val, inp);
#else
    return sensor_value_from_float(val, inp);
#endif
}

or_scalar_t or_lerp(or_scalar_t a, or_scalar_t b, or_scalar_t t) {
    assert(0 <= t && t <= 1.0);
    return a * (1.0 - t) + b * t;
}

or_scalar_t or_get_time(unsigned int sampling_period_us, unsigned int lag_time_us) {
    int64_t us = k_ticks_to_us_near64(k_uptime_ticks());
    if (sampling_period_us != 0) {
        us = (us / sampling_period_us) * sampling_period_us;
    }
    us -= lag_time_us;
    us -= CONFIG_OPENROCKET_MS_BEFORE_LAUNCH * 1000;
    return ((or_scalar_t) (us)) / 1000000.0;
}

void or_find_bounding_packets(unsigned int last_lower_idx, or_scalar_t or_time, unsigned int* lower_idx,
                              unsigned int* upper_idx, or_scalar_t* mix) {
    // Simulation doesn't have information at this point
    if (or_time < 0) {
        *lower_idx = 0;
        *upper_idx = 0;
        *mix = 0;
        return;
    }

    unsigned int i = last_lower_idx;
    while (or_time > or_packets[i + 1].time_s) {
        if (i >= or_packets_size - 2) {
            // We've gone past what the simulation measured.
            *lower_idx = or_packets_size;
            *upper_idx = or_packets_size;
            *mix = 0;
            return;
        }
        i++;
    }

    *lower_idx = i;
    *upper_idx = i + 1;
    or_scalar_t lo = or_packets[i].time_s;
    or_scalar_t hi = or_packets[i + 1].time_s;
    *mix = (or_time - lo) / (hi - lo);
}

#ifdef CONFIG_OPENROCKET_EVENT_LOG
static const char* event_to_str(enum or_event_t e) {
    if (e < 0 || e > OR_EVENT_SIMULATION_END) {
        return "INVALID EVENT";
    }
    static const char* names[] = {
        [OR_EVENT_IGNITION] = "IGNITION",     [OR_EVENT_LAUNCH] = "LAUNCH",
        [OR_EVENT_LIFTOFF] = "LIFTOFF",       [OR_EVENT_LAUNCHROD] = "LAUNCHROD",
        [OR_EVENT_BURNOUT] = "BURNOUT",       [OR_EVENT_EJECTION_CHARGE] = "EJECTION_CHARGE",
        [OR_EVENT_APOGEE] = "APOGEE",         [OR_EVENT_RECOVERY_DEVICE_DEPLOYMENT] = "RECOVERY_DEVICE_DEPLOYMENT",
        [OR_EVENT_GROUND_HIT] = "GROUND_HIT", [OR_EVENT_SIMULATION_END] = "SIMULATION_END",

    };
    return names[e];
}
static void or_event_thread_handler(void) {
    // This thread starts at T=0 via the zephyr thread startup time
    or_scalar_t time = 0;
    unsigned int i = 0;
    while (i < or_events_size - 1) {
        int time_to_wait_ms = (int) ((or_events[i].time_s - time) * 1000);
        k_msleep(time_to_wait_ms);
        time = or_events[i].time_s;
        LOG_INF("OpenRocket event %s at time T+%.3f", event_to_str(or_events[i].event), or_events[i].time_s);
        i++;
    }
    LOG_INF("OpenRocket flight over");
}
#endif

static struct or_data_t pad_packet = {
    .time_s = 0,
#ifdef CONFIG_OPENROCKET_IMU
    .vert_accel = 9.801,
    .lat_accel = 0,
    .roll = 0,
    .pitch = 0,
    .yaw = 0,
#endif
#ifdef CONFIG_OPENROCKET_BAROMETER
    .pressure = 0,
    .temperature = 0,
#endif
#ifdef CONFIG_OPENROCKET_GNSS
    .latitude = or_packets[0].latitude,
    .longitude = or_packets[0].longitude,
    .altitude = or_packets[0].altitude,
    .velocity = 0,
#endif

};

static struct or_data_t landed_packet = {
    .time_s = 0,
#ifdef CONFIG_OPENROCKET_IMU
    .vert_accel = 9.801,
    .lat_accel = 0,
    .roll = 0,
    .pitch = 0,
    .yaw = 0,
#endif
#ifdef CONFIG_OPENROCKET_BAROMETER
    .pressure = 0,
    .temperature = 0,
#endif
#ifdef CONFIG_OPENROCKET_GNSS
    .latitude = 0,
    .longitude = 0,
    .altitude = 0,
    .velocity = 0,
#endif

};

static int init_openrocket(void) {
    LOG_INF("Initializing OpenRocket data");
#ifdef CONFIG_OPENROCKET_BAROMETER
    pad_packet.pressure = or_packets[0].temperature;
    pad_packet.temperature = or_packets[0].temperature;
    landed_packet.pressure = or_packets[or_packets_size - 1].pressure;
    landed_packet.temperature = or_packets[or_packets_size - 1].temperature;
#endif

#ifdef CONFIG_OPENROCKET_GNSS
    pad_packet.latitude = or_packets[0].latitude;
    pad_packet.longitude = or_packets[0].longitude;
    pad_packet.altitude = or_packets[0].altitude;
    landed_packet.latitude = or_packets[or_packets_size - 1].latitude;
    landed_packet.longitude = or_packets[or_packets_size - 1].longitude;
    landed_packet.altitude = or_packets[or_packets_size - 1].altitude;
#endif
    return 0;
}

void or_get_presim(struct or_data_t* packet) { *packet = pad_packet; }
void or_get_postsim(struct or_data_t* packet) { *packet = landed_packet; }
