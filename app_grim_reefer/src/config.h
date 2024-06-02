#ifndef REEFER_INCLUDE_CONFIG_H
#define REEFER_INCLUDE_CONFIG_H

#include <zephyr/kernel.h>

// Use buzzer or led for status
#define PEOPLE_ARE_SLEEPING

// Storage Thread
#define STORAGE_QUEUE_SIZE        150
#define STORAGE_THREAD_STACK_SIZE 2048
#define STORAGE_THREAD_PRIORITY   25

// Sensor Reading
#define FAST_DATA_DELAY_MS K_MSEC(2)
#define SLOW_DATA_DELAY_MS K_MSEC(1000)

#define TOTAL_FLIGHT_TIME_MS (400 * 1000)

// Boost Detection
#define ALTITUDE_BUFFER_SIZE    500
#define ALTITUDE_TIME_THRESHOLD K_SECONDS(5)
#define ALTITUDE_VAL_THRESHOLD  500

#define ACCEL_BUFFER_SIZE    250
#define ACCEL_TIME_THRESHOLD K_MSEC(250)
#define ACCEL_VAL_THRESHOLD  ((float) (9.81 * 5))

#define DETECTION_METHOD_PER_SENSOR_COUNT 2

// FS Setup
#define ADC_FILENAME  "/lfs/adc.bin"
#define FAST_FILENAME "/lfs/fast.bin"
#define SLOW_FILENAME "/lfs/slow.bin"

#define PRELAUNCH_ACCEL_FILENAME "/lfs/pre_accel.bin"
#define PRELAUNCH_ALT_FILENAME   "/lfs/pre_accel.bin"
#define PRELAUNCH_TEMP_FILENAME  "/lfs/pre_accel.bin"

#endif
