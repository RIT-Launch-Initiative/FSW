#include <zephyr/kernel.h>

#define LOGGING_STACK_SIZE 512

static void logging_task(void);
K_THREAD_DEFINE(data_logger, LOGGING_STACK_SIZE, logging_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

static void init_logging(void) {

}

static void logging_task(void) {
    init_logging();

    while (true) {

    }
}
