// Launch Includes
#include <launch_core/types.h>
#include <launch_core/os/fs.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(data_logging);

#define LOGGING_STACK_SIZE 1024
#define LOGGING_PRIORITY K_PRIO_PREEMPT(20)

// Threads
static void log_telemetry_task(void);
K_THREAD_DEFINE(logging, LOGGING_STACK_SIZE, log_telemetry_task, NULL, NULL, NULL, LOGGING_PRIORITY, 0, 1000);

// Extern Variables
extern struct k_msgq hundred_hz_log_queue;

// Global Variables
bool logging_enabled = false;

static void log_telemetry_task(void) {
    sensor_module_hundred_hz_telemetry_t telem;
    while (true) {
        if (logging_enabled && k_msgq_get(&hundred_hz_log_queue, &telem, K_FOREVER)) {
            // TODO: Wipe this once we actually call loggign stuff
            LOG_INF("Logged sensor module data");
        }
    }
}

