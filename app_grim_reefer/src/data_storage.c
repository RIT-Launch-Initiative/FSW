#include "data_storage.h"
#include "config.h"
#include "orchestrator.h"
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>

LOG_MODULE_REGISTER(storage);

// Initialize message queues

#define QUEUE_SIZE 2
K_MSGQ_DEFINE(slow_data_queue, sizeof(struct slow_data), QUEUE_SIZE, 1);
K_MSGQ_DEFINE(adc_data_queue, sizeof(struct adc_data), QUEUE_SIZE, 1);
K_MSGQ_DEFINE(fast_data_queue, sizeof(struct fast_data), QUEUE_SIZE, 1);

int data_writing_thread(void *, void *, void *) {

  struct fs_file_t adc_file;
  struct fs_file_t fast_file;
  struct fs_file_t slow_file;

  // Open Files (unless LFS_NO_ALLOC is set, this will allocate caches according
  // to the fstab in the board device tree)
  LOG_INF("openning data files");
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

  // local copies to read into
  struct adc_data adc_data_local;
  struct fast_data fast_data_local;
  struct slow_data slow_dat_local;
  *((int *)0) = 0;
  // Start reading
  while (1) {
    // Check to see if flight is over
    uint32_t event_status =
        k_event_wait(&flight_over, EVENT_HAPPENED, false, K_NO_WAIT);
    if (event_status == EVENT_HAPPENED) {
      break;
    }
    // save data
    int queue_ret;

    // adc data
    queue_ret = k_msgq_get(&adc_data_queue, &adc_data_local, K_NO_WAIT);
    if (queue_ret == 0) {
      // write data
      ret = fs_write(&adc_file, &adc_data_local, sizeof(adc_data_local));
      if (ret < 0) {
        LOG_ERR("error writing adc file\n");
      }
    }

    // fast data
    queue_ret = k_msgq_get(&fast_data_queue, &fast_data_local, K_NO_WAIT);
    if (queue_ret == 0) {
      // write data
      ret = fs_write(&fast_file, &fast_data_local, sizeof(fast_data_local));
      if (ret < 0) {
        LOG_ERR("error writing fast file\n");
      }
    }

    // slow data
    queue_ret = k_msgq_get(&slow_data_queue, &slow_dat_local, K_NO_WAIT);
    if (queue_ret == 0) {
      // write data
      ret = fs_write(&slow_file, &slow_dat_local, sizeof(slow_dat_local));
      if (ret < 0) {
        LOG_ERR("error writing slow file\n");
      }
    }

    k_usleep(1);
  }
  LOG_INF("flight over, saving all\n");
  // close it all
  ret = fs_close(&adc_file);
  if (ret < 0) {
    LOG_ERR("failed to close adc file\n");
  }
  ret = fs_close(&fast_file);
  if (ret < 0) {
    LOG_ERR("failed to close fast file\n");
  }

  ret = fs_close(&slow_file);
  if (ret < 0) {
    LOG_ERR("failed to close slow file\n");
  }
  return 0;
}

#define DATA_STORAGE_STACK_SIZE 500
#define DATA_STORAGE_PRIORITY 5

extern void my_entry_point(void *, void *, void *);

K_THREAD_DEFINE(data_tid, DATA_STORAGE_STACK_SIZE, data_writing_thread, NULL,
                NULL, NULL, DATA_STORAGE_PRIORITY, 0, 0);