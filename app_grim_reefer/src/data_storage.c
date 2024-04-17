#include "config.h"
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(storage);

// K_MSGQ_DEFINE(slow_data_queue, sizeof(struct slow_data), STORAGE_QUEUE_SIZE,
// 1); K_MSGQ_DEFINE(adc_data_queue, sizeof(struct adc_data),
// STORAGE_QUEUE_SIZE, 1); K_MSGQ_DEFINE(fast_data_queue, sizeof(struct
// fast_data), STORAGE_QUEUE_SIZE, 1);
//