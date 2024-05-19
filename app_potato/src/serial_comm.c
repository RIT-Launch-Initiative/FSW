#include <zephyr/kernel.h>

#define SERIAL_TASK_STACK_SIZE 512

static void receive_serial(void);
K_THREAD_DEFINE(slip_tx_thread, SERIAL_TASK_STACK_SIZE, receive_serial, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

static bool init_serial() {

    return true;
}

static void receive_serial(void) {
    if (!init_serial()) {
        return;
    }

    while (true) {

    }
}
