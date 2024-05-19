// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define SERIAL_TASK_STACK_SIZE 512

LOG_MODULE_REGISTER(serial_comm);

// Threads
static void receive_serial(void);
K_THREAD_DEFINE(serial_rx_thread, SERIAL_TASK_STACK_SIZE, receive_serial, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

static bool init_serial() {

    return true;
}

static void receive_serial(void) {
    uint8_t receive_char = 0;

    if (!init_serial()) {
        return;
    }

    while (true) {
        // TODO: Read from serial

        switch (receive_char) {

        }
    }
}
