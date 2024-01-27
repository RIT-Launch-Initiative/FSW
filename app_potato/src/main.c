#include <app_version.h>

#include <launch_core/device_utils.h>
#include <launch_core/net_utils.h> // TODO: Might need for SLIP

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_POTATO_LOG_LEVEL
);

// Threads
#define POTATO_STACK_SIZE (512)

static K_THREAD_STACK_DEFINE(adc_read_stack, POTATO_STACK_SIZE);
static struct k_thread adc_read_thread;

static K_THREAD_STACK_DEFINE(sensor_read_stack, POTATO_STACK_SIZE);
static struct k_thread sensor_read_thread;

static K_THREAD_STACK_DEFINE(slip_tx_stack, POTATO_STACK_SIZE);
static struct k_thread slip_tx_thread;

// Queues
K_QUEUE_DEFINE(slip_tx_queue);

void adc_read_task(void *, void *, void *) {
    // Check ADC


    // Do ADC stuff
    while (1) {

    }
}


void sensor_read_task(void *, void *, void *) {
    // Check devices

    // Do sensor stuff
    while (1) {

    }
}


void slip_tx_task(void *, void *, void *) {
    while (1) {

    }
}

static int init(void) {
    // Initialize SLIP

    // Arbitrate with connected module over SLIP
    // TODO: Modify condition. Loop should exit when acknowledgement is sent by parent module
    while (false) {

    }

    // Initialize tasks
    // TODO: Maybe prioritize in this order (ADC, SLIP, sensors)
    k_thread_create(&adc_read_thread, adc_read_stack, POTATO_STACK_SIZE, adc_read_task, NULL, NULL,
                    NULL, 0, 0, K_NO_WAIT);
    k_thread_create(&sensor_read_thread, sensor_read_stack, POTATO_STACK_SIZE, sensor_read_task, NULL, NULL,
                    NULL, 0, 0, K_NO_WAIT);
    k_thread_create(&slip_tx_thread, slip_tx_stack, POTATO_STACK_SIZE, slip_tx_task, NULL, NULL,
                    NULL, 0, 0, K_NO_WAIT);

    k_thread_start(&adc_read_thread);
    k_thread_start(&sensor_read_thread);
    k_thread_start(&slip_tx_thread);

    return 0;
}

int main() {
    if (!init()) {
        return -1;
    }

    return 0;
}


