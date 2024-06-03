#include "data_storage.h"

#include "config.h"

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(storage);

enum control_event { BEGIN_STORAGE, FINISH_STORAGE };

// Communication channels
K_MSGQ_DEFINE(slow_data_queue, sizeof(struct slow_data), STORAGE_QUEUE_SIZE, 1);
K_MSGQ_DEFINE(adc_data_queue, sizeof(struct adc_data), STORAGE_QUEUE_SIZE, 1);
K_MSGQ_DEFINE(fast_data_queue, sizeof(struct fast_data), STORAGE_QUEUE_SIZE, 1);
K_MSGQ_DEFINE(data_storage_control_queue, sizeof(enum control_event), 5, 1);

#define STORAGE_SETUP_SUCCESS_EVENT 0x1
#define STORAGE_SETUP_FAILED_EVENT  0x2
K_EVENT_DEFINE(storage_setup_finished);

// Events that this thread handles
#define NUM_EVENTS 4
static struct k_poll_event events[NUM_EVENTS] = {
    K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &slow_data_queue, 0),
    K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &adc_data_queue, 0),
    K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &fast_data_queue, 0),
    K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY,
                                    &data_storage_control_queue, 0),

};

// Forward Declarations
void storage_thread_entry_point(void);

// Threads
K_THREAD_DEFINE(storage_thread, STORAGE_THREAD_STACK_SIZE, storage_thread_entry_point, NULL, NULL, NULL,
                STORAGE_THREAD_PRIORITY, 0, 1000);

// Helpers to deal with doing the same thing for many files
#define CHECK_AND_STORE(event, data, file, filename)                                                                   \
    if (event.state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {                                                             \
        k_msgq_get(event.msgq, &data, K_NO_WAIT);                                                                      \
        int ret = fs_write(&file, (uint8_t *) (&data), sizeof(data));                                                  \
        if (ret < 0) {                                                                                                 \
            LOG_INF("Error writing %s", filename);                                                                     \
        }                                                                                                              \
    }                                                                                                                  \
                                                                                                                       \
    event.state = K_POLL_STATE_NOT_READY;

#define OPEN_OR_FAIL(name, filename)                                                                                   \
    struct fs_file_t name;                                                                                             \
    {                                                                                                                  \
        fs_file_t_init(&name);                                                                                         \
        int ret = fs_open(&name, filename, FS_O_RDWR | FS_O_CREATE);                                                   \
                                                                                                                       \
        if (ret < 0) {                                                                                                 \
            LOG_ERR("Error opening %s. %d", filename, ret);                                                            \
            k_event_post(&storage_setup_finished, STORAGE_SETUP_FAILED_EVENT);                                         \
            return;                                                                                                    \
        }                                                                                                              \
    }

void storage_thread_entry_point() {
    struct fast_data fast_dat = {0};
    struct slow_data slow_dat = {0};
    struct adc_data adc_dat = {0};
    enum control_event event = 0;
    int ret;

    do {
        k_msgq_get(&data_storage_control_queue, &event, K_FOREVER);
    } while (event != BEGIN_STORAGE);

    OPEN_OR_FAIL(fast_file, FAST_FILENAME);
    OPEN_OR_FAIL(slow_file, SLOW_FILENAME);
    OPEN_OR_FAIL(adc_file, ADC_FILENAME);

    k_event_post(&storage_setup_finished, STORAGE_SETUP_SUCCESS_EVENT);

    while (true) {
        k_poll(events, NUM_EVENTS, K_FOREVER);

        // Slow data
        CHECK_AND_STORE(events[0], slow_dat, slow_file, SLOW_FILENAME)
        // ADC data
        CHECK_AND_STORE(events[1], adc_dat, adc_file, ADC_FILENAME)
        // Fast data
        CHECK_AND_STORE(events[2], fast_dat, fast_file, FAST_FILENAME)

        if (events[3].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
            k_msgq_get(events[3].msgq, &event, K_NO_WAIT);
            if (event == FINISH_STORAGE) {
                break;
            }
        }
    }
    LOG_INF("Flight over. Saving files...");
    ret = fs_close(&fast_file);
    if (ret < 0) {
        LOG_ERR("Failed to save fast file. %d", ret);
    }
    ret = fs_close(&slow_file);
    if (ret < 0) {
        LOG_ERR("Failed to save slow file. %d", ret);
    }
    ret = fs_close(&adc_file);
    if (ret < 0) {
        LOG_ERR("Failed to save adc file. %d", ret);
    }

    LOG_INF("Saved Files");
}

#define EVENT_FILTER_ALL 0xFFFFFFFF

int start_data_storage_thread() {
    enum control_event event = BEGIN_STORAGE;
    k_msgq_put(&data_storage_control_queue, &event, K_FOREVER);
    if (k_event_wait(&storage_setup_finished, EVENT_FILTER_ALL, false, K_FOREVER) == STORAGE_SETUP_FAILED_EVENT) {
        // VERY VERY BAD - payload will log no data
        return -1;
    }
    return 0;
}

void finish_data_storage() {
    enum control_event event = FINISH_STORAGE;
    k_msgq_put(&data_storage_control_queue, &event, K_FOREVER);
    k_msleep(1000);
}