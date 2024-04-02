// Self Include
#include "sensors.h"

// stdlib Includes
#include <stdint.h>

// Launch Includes
#include <launch_core/types.h>

// Zephyr Includes
#include <zephyr/kernel.h>

// Constants
#define SENSOR_READING_STACK_SIZE 1024
#define HUNDRED_HZ_TELEM_PRIORITY 10

// Forward Declarations
static void hundred_hz_sensor_reading_task(void *unused0, void *unused1, void *unused2);

// Threads
K_THREAD_DEFINE(hundred_hz_readings, SENSOR_READING_STACK_SIZE, hundred_hz_sensor_reading_task, NULL, NULL, NULL,
                HUNDRED_HZ_TELEM_PRIORITY, 0, 0);

// Message Queues
K_MSGQ_DEFINE(hundred_hz_telem_queue, sizeof(sensor_module_hundred_hz_telemetry_t), 16, 1);

static void hundred_hz_sensor_reading_task(void *unused0, void *unused1, void *unused2) {

}
