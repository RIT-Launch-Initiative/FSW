#include "config.h"

#include "data_storage.h"
#include "flight.h"

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(storage);

K_EVENT_DEFINE(storage_setup_finished);

K_MSGQ_DEFINE(slow_data_queue, sizeof(struct slow_data), STORAGE_QUEUE_SIZE, 1);
K_MSGQ_DEFINE(adc_data_queue, sizeof(struct adc_data), STORAGE_QUEUE_SIZE, 1);
K_MSGQ_DEFINE(fast_data_queue, sizeof(struct fast_data), STORAGE_QUEUE_SIZE, 1);
K_MSGQ_DEFINE(flight_events_queue, sizeof(enum flight_event), 5, 1);

#define NUM_EVENTS 4
static struct k_poll_event events[NUM_EVENTS] = {
    K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
                                    K_POLL_MODE_NOTIFY_ONLY, &slow_data_queue,
                                    0),
    K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
                                    K_POLL_MODE_NOTIFY_ONLY, &adc_data_queue,
                                    0),
    K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
                                    K_POLL_MODE_NOTIFY_ONLY, &fast_data_queue,
                                    0),
    K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
                                    K_POLL_MODE_NOTIFY_ONLY,
                                    &flight_events_queue, 0),

};

#define CHECK_AND_STORE(event, buf, file, filename)                            \
  if (event.state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {                       \
    k_msgq_get(event.msgq, &buf, K_NO_WAIT);                                   \
    int ret = fs_write(&file, &buf, sizeof(buf));                              \
    if (ret < 0) {                                                             \
      LOG_INF("Error writing %s", filename);                                   \
    }                                                                          \
  }                                                                            \
  \ 
  event.state = K_POLL_STATE_NOT_READY;

void storage_thread_entry_point(void *, void *, void *) {
  struct fast_data fast_dat;
  struct slow_data slow_dat;
  struct adc_data adc_dat;
  enum flight_event event;

  struct fs_file_t fast_file;
  fs_file_t_init(&fast_file);
  int ret = fs_open(&fast_file, "/lfs/fast.bin", FS_O_RDWR | FS_O_CREATE);
  if (ret < 0) {
    LOG_ERR("Failed to open %s", "/lfs/fast.bin");
    k_event_post(&storage_setup_finished, STORAGE_SETUP_FAILED_EVENT);
    return;
  }

  k_event_post(&storage_setup_finished, STORAGE_SETUP_SUCCESS_EVENT);

  for (;;) {
    k_poll(events, NUM_EVENTS, K_FOREVER);

    // Slow data
    CHECK_AND_STORE(events[0], slow_dat, fast_file, "/lfs/fast.bin")
    // ADC data
    CHECK_AND_STORE(events[1], adc_dat, fast_file, "/lfs/fast.bin")
    // Fast data
    CHECK_AND_STORE(events[2], fast_dat, fast_file, "/lfs/fast.bin")

    if (events[3].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
      k_msgq_get(events[3].msgq, &event, K_NO_WAIT);
      if (event == flight_event_main_shutoff) {
        break;
      }
    }
  }
  LOG_INF("Flight over. Saving files...");
  ret = fs_close(&fast_file);
  if (ret < 0) {
    LOG_ERR("Failed to close file. Uh oh");
  }
}

// Setup thread

K_THREAD_STACK_DEFINE(storage_thread_stack_area, STORAGE_THREAD_STACK_SIZE);
struct k_thread storage_thread_data;

k_tid_t spawn_data_storage_thread() {
  k_tid_t my_tid =
      k_thread_create(&storage_thread_data, storage_thread_stack_area,
                      K_THREAD_STACK_SIZEOF(storage_thread_stack_area),
                      storage_thread_entry_point, NULL, NULL, NULL,
                      STORAGE_THREAD_PRIORITY, 0, K_NO_WAIT);
  k_thread_name_set(my_tid, "storage");
  return my_tid;
}