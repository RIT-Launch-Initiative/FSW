#ifndef REEFER_INCLUDE_CONFIG_H
#define REEFER_INCLUDE_CONFIG_H

#include <zephyr/kernel.h>

#define STORAGE_QUEUE_SIZE 150

#define STORAGE_THREAD_STACK_SIZE 2048
#define STORAGE_THREAD_PRIORITY   25

#define FAST_DATA_DELAY_MS K_MSEC(2)
#define SLOW_DATA_DELAY_MS K_MSEC(1000)

#define TOTAL_FLIGHT_TIME_MS (400 * 1000)

#define ADC_FILENAME  "/lfs/adc.bin"
#define FAST_FILENAME "/lfs/fast.bin"
#define SLOW_FILENAME "/lfs/slow.bin"

#endif
