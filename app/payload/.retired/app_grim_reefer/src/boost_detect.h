#ifndef REEFER_INLCUDE_BOOST_DETECT_H
#define REEFER_INLCUDE_BOOST_DETECT_H

#include <zephyr/device.h>
/**
 * start_boost_detect
 * Tell the system to begin watching for launch
 * Detection parameters defined in config.h
 * @param imu the imu device to watch for the spike in acceleration
 * @param altimeter the altimeter device to watch for a fast change in height
*/
void start_boost_detect(const struct device* imu, const struct device* altimeter, const struct device* battery_ina);
/**
 * stop_boost_detect
 * Tell the system to stop watching for boost detection
 * Use this if boost has been detected or you wish to cancel launch detection
*/
void stop_boost_detect();

/**
 * get_boost_detected
 * @return true if the vehicle has launched
*/
bool get_boost_detected();

/**
 * Flush the launch detection buffers to disk
 * Filenames determined by config.h
*/
void save_boost_data();

#endif