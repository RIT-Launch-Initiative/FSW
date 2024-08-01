#include "c_print_count.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(counter);

CPublisher::CPublisher(const char* name, int *count) : CTenant(name), count(count) {
}

void CPublisher::Startup() {
    CBase::Startup(); // Initialize any parent functionality

    *count = 0;
}

void CPublisher::Run() {
    *count += 1;
    LOG_INF("%s: %d", name, *count);
}


