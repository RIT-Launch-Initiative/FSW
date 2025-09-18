#include "storage.h"

#include "common.h"
#include "f_core/os/flight_log.hpp"
#include "flight.h"
#include "gorbfs.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(storage);

// Flash Targets

int storage_thread_entry(void *, void *, void *){}