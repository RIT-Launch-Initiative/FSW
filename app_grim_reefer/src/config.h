#pragma once
#include <zephyr/kernel.h>

#define STORAGE_QUEUE_SIZE 100

#define STORAGE_THREAD_STACK_SIZE 2048
#define STORAGE_THREAD_PRIORITY   25

#define FAST_DATA_DELAY_MS K_MSEC(2)
#define SLOW_DATA_DELAY_MS K_MSEC(1000)

// Openrocket predicts 75 second flight time for bare L1

#define FLIGHT_TIME_MS (110 * 1000)

#define FIVE_G_LIMIT_MPS 50
// 500 samples a sec max
// 500 / 4 per quarter second
// 500/4
#define ACC_BOOST_DETECTION_AVG_SAMPLES 100

// gyro sens 70
// accel sens 0.488
// 	dval = (double)(raw_val) * (double)sensitivity * SENSOR_G_DOUBLE / 1000;
// SENSOR_G = 9806650LL
// #define SENSOR_G_DOUBLE				(SENSOR_G / 1000000.0)

#define ADC_FILENAME  "/lfs/adc.bin"
#define FAST_FILENAME "/lfs/fast.bin"
#define SLOW_FILENAME "/lfs/slow.bin"