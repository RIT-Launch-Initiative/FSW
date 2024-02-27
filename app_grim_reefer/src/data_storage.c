#include "data_storage.h"
#include "config.h"
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>

LOG_MODULE_REGISTER(storage);

// Initialize message queues

#define QUEUE_SIZE 2
K_MSGQ_DEFINE(slow_data_queue, sizeof(struct slow_data), QUEUE_SIZE, 1);
K_MSGQ_DEFINE(adc_data_queue, sizeof(struct adc_data), QUEUE_SIZE, 1);
K_MSGQ_DEFINE(fast_data_queue, sizeof(struct fast_data), QUEUE_SIZE, 1);

int data_writing_thread() {
  // local copies to read into
  struct adc_data adc_dat;
  struct fast_data fast_dat;
  struct slow_data slow_dat;

  struct fs_file_t adc_file;
  struct fs_file_t fast_file;
  struct fs_file_t slow_file;

  // Open Files
  int ret;
  fs_file_t_init(&adc_file);
  ret = fs_open(&adc_file, DATA_ADC_FILEPATH, FS_O_CREATE | FS_O_RDWR);
  if (ret < 0) {
    LOG_ERR("Failed to open %s: %d\n", DATA_ADC_FILEPATH, ret);
    return ret;
  }

  fs_file_t_init(&fast_file);
  ret = fs_open(&fast_file, DATA_FAST_FILEPATH, FS_O_CREATE | FS_O_RDWR);
  if (ret < 0) {
    LOG_ERR("Failed to open %s: %d\n", DATA_FAST_FILEPATH, ret);
    return ret;
  }

  fs_file_t_init(&slow_file);
  ret = fs_open(&slow_file, DATA_SLOW_FILEPATH, FS_O_CREATE | FS_O_RDWR);
  if (ret < 0) {
    LOG_ERR("Failed to open %s: %d\n", DATA_SLOW_FILEPATH, ret);
    return ret;
  }

  // Start reading
  while (1) {
    int queue_ret;
    // adc data
    queue_ret = k_msgq_get(&adc_data_queue, &adc_dat, K_NO_WAIT);
    if (queue_ret == 0) {
      // write data
    }
    // fast data
    queue_ret = k_msgq_get(&fast_data_queue, &fast_dat, K_NO_WAIT);
    if (queue_ret == 0) {
      // write data
    }
    // slow data
    queue_ret = k_msgq_get(&slow_data_queue, &slow_dat, K_NO_WAIT);
    if (queue_ret == 0) {
      // write data
    }

    // will return immediatly if no other threads need to run
    k_yield();
  }
  return 0;
}