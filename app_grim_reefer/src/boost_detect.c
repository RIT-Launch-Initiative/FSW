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
static void altitude_boost_reading_task(void);
static void accel_boost_reading_task(void);

// Timers
K_TIMER_DEFINE(altitude_boost_detect_timer, NULL, NULL);
K_TIMER_DEFINE(accel_boost_detect_timer, NULL, NULL);

// Threads
K_THREAD_DEFINE(altimeter_boost_thread, 1024, altitude_boost_reading_task, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0,
                1000);
K_THREAD_DEFINE(accel_boost_thread, 1024, accel_boost_reading_task, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0, 1000);

// Dependencies to be injected
const struct device* imu_dev = NULL;
const struct device* altimeter_dev = NULL;

// Rolling buffers for data recovery
bool boost_detected = false;

struct fast_data accel_buffer[ACCEL_BUFFER_SIZE];
int accel_buffer_index = 0;

l_barometer_data_t altitude_buffer[ALTITUDE_BUFFER_SIZE];
int altitude_buffer_index;

static void altitude_boost_reading_task(void) {
    while (true) {
        k_timer_status_sync(&altitude_boost_detect_timer);
        if (altimeter_dev == NULL) {
            LOG_ERR("No altitude device given to detect launch with");
            return;
        }
        // Read sensor, save to buffer
        printk("Read sensor\n");
    }
}

static void accel_boost_reading_task(void) {
    while (true) {
        k_timer_status_sync(&accel_boost_detect_timer);

        if (altimeter_dev == NULL) {
            LOG_ERR("No IMU device given to detect launch with");
            return;
        }
    }
}

void start_boost_detect(const struct device* imu, const struct device* altimeter) {
    LOG_INF("Starting Boost Detection");
    imu_dev = imu;
    altimeter_dev = altimeter;
    k_timer_start(&accel_boost_detect_timer, K_MSEC(1), K_MSEC(1));
    k_timer_start(&altitude_boost_detect_timer, K_MSEC(10), K_MSEC(10));
}

void stop_boost_detect() {
    LOG_INF("Ending Boost Detection");
    k_timer_stop(&altitude_boost_detect_timer);
    k_timer_stop(&accel_boost_detect_timer);
    imu_dev = NULL;
    altimeter_dev = NULL;
}

inline bool get_boost_detected() { return boost_detected; }