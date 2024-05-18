#include "sensor_module.h"

#include <zephyr/kernel.h>

#define ALTITUDE_TIME_THRESHOLD K_SECONDS(5)
#define ALTITUDE_VAL_THRESHOLD 500

#define ACCEL_TIME_THRESHOLD K_MSEC(250)
#define ACCEL_VAL_THRESHOLD 9.81 * 5


// Forward Declares
static void altitude_boost_detect_cb(struct k_timer*);
static void accel_boost_detect_cb(struct k_timer*);

// Timers
K_TIMER_DEFINE(altitude_boost_detect_timer, altitude_boost_detect_cb, NULL);
K_TIMER_DEFINE(accel_boost_detect_timer, accel_boost_detect_cb, NULL);

bool boost_detected = false;
extern float accel_z[DETECTION_METHOD_PER_SENSOR_COUNT];
extern float pressure[DETECTION_METHOD_PER_SENSOR_COUNT];
extern float temperature[DETECTION_METHOD_PER_SENSOR_COUNT];

static void altitude_boost_detect_cb(struct k_timer *) {
    // TODO: Function for calculating altitude from press and temp
    static float prev_altitudes[2];

    for (int i = 0; i < DETECTION_METHOD_PER_SENSOR_COUNT; i++) {
        if ((pressure[i] - prev_altitudes[i]) < ALTITUDE_VAL_THRESHOLD) {
            prev_altitudes[i] = pressure[i];
            continue;
        }

        boost_detected = true;
        return;
    }
}

static void accel_boost_detect_cb(struct k_timer *) {
    static float prev_accel_z[2];
    for (int i = 0; i < DETECTION_METHOD_PER_SENSOR_COUNT; i++) {
        if ((accel_z[i] - prev_accel_z[i]) < ACCEL_VAL_THRESHOLD) {
            prev_accel_z[i] = accel_z[i];
            continue;
        }

        boost_detected = true;
        return;
    }
}

void init_boost_detect() {
    k_timer_start(&altitude_boost_detect_timer, ALTITUDE_TIME_THRESHOLD, ALTITUDE_TIME_THRESHOLD);
    k_timer_start(&accel_boost_detect_timer, ACCEL_TIME_THRESHOLD, ACCEL_TIME_THRESHOLD);
}

void stop_boost_detect() {
    k_timer_stop(&altitude_boost_detect_timer);
    k_timer_stop(&accel_boost_detect_timer);
}

inline bool get_boost_detected() {
    return boost_detected;
}