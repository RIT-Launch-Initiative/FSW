#include "sensor_module.h"

#include <zephyr/kernel.h>


static bool boost_detected = false;
inline bool get_boost_detected() {
    return boost_detected;
}

/**
 * Check for a sudden altitude change and set boost_detected if threshold met
 */
void check_altitude_change(float pressure, float temperature) {

}

/**
 * Check for a sudden acceleration change and set boost_detected if threshold met
 */
void check_acceleration_change(float accel_z) {

}

static void boost_detection_task(void) {

}
