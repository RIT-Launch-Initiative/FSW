#include "boost_detect.h"

#include "config.h"
#include "data_storage.h"

// Launch Core Includes
#include <launch_core/conversions.h>
#include <launch_core/types.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(boost_detect, CONFIG_APP_GRIM_REEFER_LOG_LEVEL);

// Forward Declares
static void altitude_boost_detect_cb(struct k_timer*);
static void accel_boost_detect_cb(struct k_timer*);

// Timers
K_TIMER_DEFINE(altitude_boost_detect_timer, altitude_boost_detect_cb, NULL);
K_TIMER_DEFINE(accel_boost_detect_timer, accel_boost_detect_cb, NULL);

// Dependencies to be injected
const struct device* imu_dev = NULL;
const struct device* altimeter_dev = NULL;

// Rolling buffers for data recovery
bool boost_detected = false;
struct fast_data accel_buffer[ACCEL_BUFFER_SIZE];
int accel_buffer_index = 0;

l_barometer_data_t altitude_buffer[ALTITUDE_BUFFER_SIZE];
int altitude_buffer_index;

static void altitude_boost_detect_cb(struct k_timer*) {
    if (altimeter_dev == NULL) {
        LOG_ERR("No altitude device given to detect launch with");
        return;
    }
    // Read sensor, save to buffer
}

static void accel_boost_detect_cb(struct k_timer*) {
    if (altimeter_dev == NULL) {
        LOG_ERR("No IMU device given to detect launch with");
        return;
    }
}

void start_boost_detect(const struct device* imu, const struct device* altimeter) {
    LOG_INF("Starting Boost Detection\n");
    imu_dev = imu;
    altimeter_dev = altimeter;
    k_timer_start(&altitude_boost_detect_timer, ALTITUDE_TIME_THRESHOLD, ALTITUDE_TIME_THRESHOLD);
    k_timer_start(&accel_boost_detect_timer, ACCEL_TIME_THRESHOLD, ACCEL_TIME_THRESHOLD);
}

void stop_boost_detect() {
    LOG_INF("Ending Boost Detection\n");
    k_timer_stop(&altitude_boost_detect_timer);
    k_timer_stop(&accel_boost_detect_timer);
    imu_dev = NULL;
    altimeter_dev = NULL;
}

inline bool get_boost_detected() { return boost_detected; }