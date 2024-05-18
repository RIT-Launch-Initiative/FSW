// Launch Includes
#include <launch_core/types.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LOGGING_STACK_SIZE 512

LOG_MODULE_REGISTER(logging);

static void logging_task(void);
K_THREAD_DEFINE(data_logger, LOGGING_STACK_SIZE, logging_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

// Message queues
// TODO: Avoid duplicate queues. Fine for now since this isn't too expensive and we have the memory.
// Easier for now since we also need to buffer data before launch
K_MSGQ_DEFINE(ina_logging_msgq, sizeof(power_module_telemetry_t), 50, 4);
K_MSGQ_DEFINE(adc_logging_msgq, sizeof(float), 200, 4);

static void init_logging(void) {}

extern bool logging_enabled;

static void logging_task(void) {
    power_module_telemetry_t sensor_telemetry;
    float vin_adc_data_v;

    init_logging();

    while (true) {
        if (!logging_enabled) {
            continue;
        }

        if (!k_msgq_get(&ina_logging_msgq, &sensor_telemetry, K_MSEC(10))) {
            LOG_INF("Logged INA219 data");
        }

        if (!k_msgq_get(&adc_logging_msgq, &vin_adc_data_v, K_MSEC(3))) {
            LOG_INF("Logged ADC data");
        }
    }
}
