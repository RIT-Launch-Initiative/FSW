#include "openrocket_sensors.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(openrocket, CONFIG_OPENROCKET_LOG_LEVEL);

extern struct or_data_t* or_packets;
extern struct or_event_occurance_t* or_events;

extern const unsigned int or_events_size;
extern const unsigned int or_packets_size;

static const char* event_to_str(enum or_event_t e);
static void print_packet(const struct or_data_t* d);
static struct or_data_t pad_packet;

int or_data_interpolator() {
    for (int i = 0; i < or_events_size; i++) {
        struct or_event_occurance_t ev = or_events[i];
        LOG_INF("Ev: %s at %f", event_to_str(ev.event), (double) ev.time_s);
    }
    LOG_INF("%d events, %d packets delayed by %d", or_events_size, or_packets_size, CONFIG_OPENROCKET_MS_BEFORE_LAUNCH);
    print_packet(&pad_packet);
    return 0;
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
};