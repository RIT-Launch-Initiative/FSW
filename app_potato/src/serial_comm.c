// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define SERIAL_TASK_STACK_SIZE 512

LOG_MODULE_REGISTER(serial_comm);

// Threads
static void receive_serial(void);
K_THREAD_DEFINE(serial_rx_thread, SERIAL_TASK_STACK_SIZE, receive_serial, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

// Global Variables
static uint8_t event_byte = 0;


uint8_t get_event_from_serial() {
    uint8_t ret = 0;

    if (event_byte) {
        ret = event_byte;
        event_byte = 0;
    }

    return ret;
}

static void receive_serial(void) {
    // TODO: Initialize RS485 here

    while (true) {
        // TODO: Read from serial and set event_byte
    }
}
