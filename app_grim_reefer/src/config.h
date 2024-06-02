#ifndef REEFER_INCLUDE_CONFIG_H
#define REEFER_INCLUDE_CONFIG_H

#include <zephyr/kernel.h>

// Use buzzer or led for status
#define PEOPLE_ARE_SLEEPING

#define DEBUG_FLIGHT

// Storage Thread
#define STORAGE_QUEUE_SIZE        150
#define STORAGE_THREAD_STACK_SIZE 2048
#define STORAGE_THREAD_PRIORITY   25

// Sensor Reading

#define ADC_DATA_DELAY  K_USEC(100)
#define FAST_DATA_DELAY K_MSEC(2)
#define SLOW_DATA_DELAY K_MSEC(1000)

// Boost Detection
#define ALTITUDE_BUFFER_SIZE    500
#define ALTITUDE_TIME_THRESHOLD K_SECONDS(5)
#define ALTITUDE_VAL_THRESHOLD  500

#define ACCEL_BUFFER_SIZE    250
#define ACCEL_TIME_THRESHOLD K_MSEC(250)
#define ACCEL_VAL_THRESHOLD  ((float) (9.81 * 5))

// Flight Events
#ifdef DEBUG_FLIGHT
#define TOTAL_FLIGHT_TIME K_SECONDS(40)
#define CAMERA_EXTRA_TIME K_SECONDS(12)
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
