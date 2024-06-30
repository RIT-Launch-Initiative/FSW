#include "openrocket_sensors.h"

#include <assert.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(openrocket, CONFIG_OPENROCKET_LOG_LEVEL);

// Forward Declarations
static void print_packet(const struct or_data_t* d);
static struct or_data_t pad_packet;

extern const struct or_data_t* const or_packets;
extern const unsigned int or_packets_size;

// Event Log forward declarations
#ifdef CONFIG_OPENROCKET_EVENT_LOG
extern const struct or_event_occurance_t* const or_events;
extern const unsigned int or_events_size;

static const char* event_to_str(enum or_event_t e);

static void or_event_thread_handler(void);
K_THREAD_DEFINE(or_event_thread, 512, or_event_thread_handler, NULL, NULL, NULL, 0, 0,
                CONFIG_OPENROCKET_MS_BEFORE_LAUNCH);
#endif

or_scalar_t or_lerp(or_scalar_t a, or_scalar_t b, or_scalar_t t) {
    assert(0 <= t && t <= 1.0);
    return a * (1.0 - t) + b * t;
}

/**
 * @brief Most times you go looking for a packet based on a time, its in between packets. 
 * @param last_lower_idx the low index from the last time you called this function (or 0 if you haven't called it yet)
 * @param or_time the time into the openrocket flight you want to view. (Lags, sampling rate, and launch delay should be computed before calling this function)
 * @param lower_idx out parameter of the index of the packet closest before the requested time
 * @param upper_idx out parameter of the index of the packet closest after the requested time
 * If the time requested is before takeoff, both lower_idx and upper_idx will be 0
 * If the time requested is after the simulation ends, both lower_idx and upper_idx will be or_packets_size
 */
void find_bounding_packets(unsigned int last_lower_idx, or_scalar_t or_time, unsigned int* lower_idx,
                           unsigned int* upper_idx) {
    // Simulation doesn't have information at this point
    if (or_time < 0) {
        *lower_idx = 0;
        *upper_idx = 0;
    }

    unsigned int i = last_lower_idx;
    do {
        if (i >= or_packets_size - 1) {
            // We've gone past what the simulation measured.
            *lower_idx = or_packets_size;
            *upper_idx = or_packets_size;
            return;
        }
        i++;
    } while (or_time > or_packets[i].time_s);

    *lower_idx = i;
    *upper_idx = i + 1;
}

static void print_packet(const struct or_data_t* d) {
    printk("Time: %.3f ", (double) d->time_s);
#ifdef CONFIG_OPENROCKET_IMU
    printk("Vert. Acc. %.3f ", (double) d->vert_accel);
    printk("Lat. Acc. %.3f ", (double) d->lat_accel);

    printk("Roll: %.3f ", (double) d->roll);
    printk("Pitch: %.3f ", (double) d->pitch);
    printk("Yaw: %.3f ", (double) d->yaw);
#endif
    printk("\n");
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
    // This thread starts at T=0
    or_scalar_t time = 0;
    unsigned int i = 0;
    while (i < or_events_size) {
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
    .vert_accel = 0,
    .lat_accel = 0,
    .roll = 0,
    .pitch = 0,
    .yaw = 0,
#endif
#ifdef CONFIG_OPENROCKET_BAROMETER
    .pressure = or_packets[0].pressure,
    .temperature = or_packets[0].temperature,
#endif
#ifdef CONFIG_OPENROCKET_GNSS
    .latitude = or_packets[0].latitude,
    .longitude = or_packets[0].longitude,
    .altitude = or_packets[0].altitude,
#endif

};