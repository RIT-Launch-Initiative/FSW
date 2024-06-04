#ifndef REEFER_INCLUDE_CONFIG_H
#define REEFER_INCLUDE_CONFIG_H

#include <zephyr/kernel.h>

// Use buzzer or led for status
#define BUZZER_USE_LED

// Use short timers so you don't get bored
#define SHORT_FLIGHT

// Use to .5 G for boost detect if directional
// doesnt effect non directional bc that can be shaken to trigger
#define EASY_BOOST_DETECT

// If here, use 1 axis boost detection, if not use magnitude
#define IMU_BOOST_DETECTION_MODE_AXIS
// which axis to use. accel_x, accel_y, accel_z
#define IMU_UP_AXIS accel_z

// Storage Thread
#define STORAGE_QUEUE_SIZE        150
#define STORAGE_THREAD_STACK_SIZE 2048
#define STORAGE_THREAD_PRIORITY   1

#define BOOST_DETECT_IMU_PRIORITY -1
#define BOOST_DETECT_ALT_PRIORITY -1

#define ADC_READ_PRIORITY  -1
#define FAST_READ_PRIORITY -1
#define SLOW_READ_PRIORITY -1

#define THREAD_START_DELAY 100

// Sensor Reading
#define ADC_DATA_DELAY   K_USEC(101) /// For some reason 100 locks up the shell
#define ALTIM_DATA_DELAY K_MSEC(10)
#define FAST_DATA_DELAY  K_MSEC(2)
#define SLOW_DATA_DELAY  K_MSEC(1000)

// Boost Detection
#define ALTITUDE_BUFFER_SIZE   500
#define ALTITUDE_VAL_THRESHOLD 500

#define ACCEL_BUFFER_SIZE 250
#ifdef EASY_BOOST_DETECT
#define ACCEL_VAL_THRESHOLD ((float) (9.81 * .5))
#else
#define ACCEL_VAL_THRESHOLD ((float) (9.81 * 5))
#endif

// Flight Events
#ifdef SHORT_FLIGHT
#define TOTAL_FLIGHT_TIME K_SECONDS(20)
#define CAMERA_EXTRA_TIME K_SECONDS(5)
#else
#define TOTAL_FLIGHT_TIME K_SECONDS(400)
#define CAMERA_EXTRA_TIME K_MINUTES(12)
#endif

// FS Setup
#define ADC_FILENAME  "/lfs/adc.bin"
#define FAST_FILENAME "/lfs/fast.bin"
#define SLOW_FILENAME "/lfs/slow.bin"

#define PRELAUNCH_ACCEL_FILENAME "/lfs/pre_imu.bin"
#define PRELAUNCH_ALT_FILENAME   "/lfs/pre_altitude.bin"

#endif
