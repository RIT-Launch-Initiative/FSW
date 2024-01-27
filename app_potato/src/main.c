#include <app_version.h>

#include <launch_core/extension_boards.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_POTATO_LOG_LEVEL
);

// Threads
#define POTATO_STACK_SIZE (512)

static K_THREAD_STACK_DEFINE(adc_read_stack, POTATO_STACK_SIZE);
static struct k_thread adc_read_thread;

static K_THREAD_STACK_DEFINE(sensor_read_stack, POTATO_STACK_SIZE);
static struct k_thread sensor_read_thread;
// TODO: Might just be a process task that sends over SLIP, but also logs data
static K_THREAD_STACK_DEFINE(slip_tx_stack, POTATO_STACK_SIZE);
static struct k_thread slip_tx_thread;

// Queues
static K_QUEUE_DEFINE(slip_tx_queue);

static void adc_read_task(void *, void *, void *) {
    // Check ADC


    // Do ADC stuff
    while (1) {

    }
}


static void sensor_read_task(void *, void *, void *) {
    // Check devices

    // Do sensor stuff
    while (1) {

    }
}


static void slip_tx_task(void *, void *, void *) {
    while (1) {

    }
}

static int init(void) {
    // Initialize SLIP

    // Arbitrate with connected module over SLIP
//    initiate_arbitration(POTATO_EXTENSION_BOARD_ID, 0);

    // Initialize tasks
    // TODO: Maybe prioritize in this order (ADC, SLIP, sensors)
//    k_thread_create(&adc_read_thread, &adc_read_stack[0], POTATO_STACK_SIZE,
//                    adc_read_task, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
//    k_thread_create(&sensor_read_thread, &sensor_read_stack[0], POTATO_STACK_SIZE,
//                    sensor_read_task, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
//    k_thread_create(&slip_tx_thread, &slip_tx_stack[0], POTATO_STACK_SIZE,
//                    slip_tx_task, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0, K_NO_WAIT);


    k_thread_start(&adc_read_thread);
    k_thread_start(&sensor_read_thread);
    k_thread_start(&slip_tx_thread);

    return 0;
}

int main() {
    init();


    return 0;
}


