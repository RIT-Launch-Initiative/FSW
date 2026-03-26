#pragma once

#include <zephyr/kernel.h>

// Use buzzer or led for status

// Use short timers so you don't get bored
#define SHORT_FLIGHT

// Use to .5 G for boost detect if directional
// doesnt effect non directional bc that can be shaken to trigger even at 5G
// #define EASY_BOOST_DETECT

// Level at which to warn low battery. in volts
#define LOW_BATTERY_VOLTAGE (4.1)

// If here, use 1 axis boost detection, if not use magnitude
#define IMU_BOOST_DETECTION_MODE_AXIS
// which axis to use. accel_x, accel_y, accel_z
#define IMU_UP_AXIS accel_z

// Boost Detection
#define ALTITUDE_BUFFER_SIZE 500
// METERS (500 ft)
#define ALTITUDE_VAL_THRESHOLD 152.4

#define ACCEL_BUFFER_SIZE 110

// ============= YOU SHOULDNT HAVE TO CHANGE ANYTHING BELOW HERE =============

#ifdef EASY_BOOST_DETECT
#define ACCEL_VAL_THRESHOLD ((float) (9.81 * .5))
#else
#define ACCEL_VAL_THRESHOLD ((float) (9.81 * 5))
#endif

// Sensor Reading
#define ADC_DATA_DELAY   K_USEC(101) /// For some reason 100 locks up the shell
#define ALTIM_DATA_DELAY K_MSEC(10)
#define FAST_DATA_DELAY  K_MSEC(2)
#define SLOW_DATA_DELAY  K_MSEC(1000)

// Flight Timing
#ifdef SHORT_FLIGHT
#define TOTAL_FLIGHT_TIME K_SECONDS(20)
#define CAMERA_EXTRA_TIME K_SECONDS(5)
#else
#define TOTAL_FLIGHT_TIME K_SECONDS(400)
#define CAMERA_EXTRA_TIME K_MINUTES(12)
#endif

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

// FS Setup
#define ADC_FILENAME  "/lfs/adc.bin"
#define FAST_FILENAME "/lfs/fast.bin"
#define SLOW_FILENAME "/lfs/slow.bin"

#define PRELAUNCH_ACCEL_FILENAME "/lfs/pre_imu.bin"
#define PRELAUNCH_ALT_FILENAME   "/lfs/pre_altitude.bin"


