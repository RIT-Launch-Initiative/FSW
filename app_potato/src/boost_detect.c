#include "potato.h"

// Launch Includes
#include <launch_core/conversions.h>

// Zephyr Includes
#include <zephyr/kernel.h>

#define ALTITUDE_TIME_THRESHOLD K_SECONDS(5)
#define ALTITUDE_VAL_THRESHOLD 500

// Forward Declares
static void altitude_boost_detect_cb(struct k_timer*);

// Timers
K_TIMER_DEFINE(altitude_boost_detect_timer, altitude_boost_detect_cb, NULL);

// Extern Variables
bool boost_detected;
extern float boost_detection_altitude;

static void altitude_boost_detect_cb(struct k_timer*) {
    static float prev_altitude = {0};
    static bool first_pass = true;

    if (first_pass && (boost_detection_altitude != -0xFFFF)) {
        prev_altitude = boost_detection_altitude;
        first_pass = false;
    }

    if ((boost_detection_altitude - prev_altitude) < ALTITUDE_VAL_THRESHOLD) {
        prev_altitude = boost_detection_altitude;
        return;
    }

    boost_detected = true;
}

void start_boost_detect() {
    k_timer_start(&altitude_boost_detect_timer, ALTITUDE_TIME_THRESHOLD, ALTITUDE_TIME_THRESHOLD);
}

void stop_boost_detect() {
    k_timer_stop(&altitude_boost_detect_timer);
}
