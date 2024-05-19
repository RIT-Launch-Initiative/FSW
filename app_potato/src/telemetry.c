#include <zephyr/kernel.h>

#define TELEMETRY_STACK_SIZE 512

static void adc_read_task(void *);
K_THREAD_DEFINE(adc_read_thread, TELEMETRY_STACK_SIZE, adc_read_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

static void sensor_read_task(void *);
K_THREAD_DEFINE(sensor_read_thread, TELEMETRY_STACK_SIZE, sensor_read_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

static void adc_read_task(void *) {
    // Check ADC

    // Do ADC stuff
    while (1) {
    }
}

static void sensor_read_task(void *) {
    // Check devices

    // Do sensor stuff
    while (1) {
    }
}
