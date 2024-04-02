#include "data_storage.h"
#include "config.h"
#include "orchestrator.h"
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(storage);

// Initialize message queues

#define QUEUE_SIZE 10
K_MSGQ_DEFINE(slow_data_queue, sizeof(struct slow_data), QUEUE_SIZE, 1);
K_MSGQ_DEFINE(adc_data_queue, sizeof(struct adc_data), QUEUE_SIZE, 1);
K_MSGQ_DEFINE(fast_data_queue, sizeof(struct fast_data), QUEUE_SIZE, 1);

#define LFS_NODE DT_ALIAS(logfs)
#define LFS_CACHE_SIZE DT_PROP(LFS_NODE, cache_size)

int read_queue_and_write(struct fs_file_t *file, struct k_msgq *queue,
                         void *local_data, unsigned int data_size) {
  int ret;

  ret = k_msgq_get(queue, local_data, K_NO_WAIT);
  if (ret == 0) {
    // write data
    ret = fs_write(file, local_data, data_size);
    if (ret < 0) {
      LOG_ERR("error writing fast file");
      return ret;
    }
  }
  return 0;
}

int init_data_file(struct fs_file_t *file, const char *fname) {
  int ret;

  fs_file_t_init(file);

  LOG_INF("initting data file: %s", fname);
  ret = fs_open(file, fname, FS_O_CREATE | FS_O_RDWR);
  if (ret < 0) {
    LOG_ERR("Failed to open %s: %d", fname, ret);
    return ret;
  }
  return 0;
}

int data_writing_thread(void *, void *, void *) {
  k_msleep(100);
  struct fs_file_t adc_file;
  struct fs_file_t fast_file;
  struct fs_file_t slow_file;

  // Open Files (unless LFS_NO_ALLOC is set, this will allocate caches according
  // to the fstab in the board device tree)
  LOG_INF("opening data files");
  int ret = 0;

  ret = init_data_file(&adc_file, DATA_ADC_FILEPATH);
  if (ret < 0) {
    return ret;
  }
  ret = init_data_file(&fast_file, DATA_FAST_FILEPATH);
  if (ret < 0) {
    return ret;
  }
  ret = init_data_file(&slow_file, DATA_SLOW_FILEPATH);
  if (ret < 0) {
    return ret;
  }

  LOG_INF("Successfully openned data files");

  // local copies to read into
  struct adc_data adc_data_local;
  struct fast_data fast_data_local;
  struct slow_data slow_dat_local;
  // Start reading
  while (1) {
    // Check to see if flight is over
    uint32_t event_status =
        k_event_wait(&flight_over, EVENT_HAPPENED, false, K_NO_WAIT);
    if (event_status == EVENT_HAPPENED) {
      break;
    }

    // save data
    int ret;

    // fast data
    ret = read_queue_and_write(&fast_file, &fast_data_queue, &fast_data_local,
                               sizeof(fast_data_local));
    if (ret < 0) {
      LOG_ERR("error writing fast file");
    }

    // adc data
    ret = read_queue_and_write(&adc_file, &adc_data_queue, &adc_data_local,
                               sizeof(adc_data_local));
    if (ret < 0) {
      LOG_ERR("error writing adc file");
    }

    // slow data
    ret = read_queue_and_write(&slow_file, &slow_data_queue, &slow_dat_local,
                               sizeof(slow_dat_local));
    if (ret < 0) {
      LOG_ERR("error writing slow file");
    }

    k_usleep(1);
  }

  LOG_INF("flight over, saving all");

  // close it all
  ret = fs_close(&adc_file);
  if (ret < 0) {
    LOG_ERR("failed to close adc file");
  }
  ret = fs_close(&fast_file);
  if (ret < 0) {
    LOG_ERR("failed to close fast file");
  }

  ret = fs_close(&slow_file);
  if (ret < 0) {
    LOG_ERR("failed to close slow file");
  }
  return 0;
}

K_THREAD_DEFINE(data_tid, DATA_STORAGE_STACK_SIZE, data_writing_thread, NULL,
                NULL, NULL, DATA_STORAGE_PRIORITY, 0, 0);