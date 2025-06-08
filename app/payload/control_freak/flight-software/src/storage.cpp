#include "common.h"
#include "f_core/os/flight_log.hpp"
#include "flight.h"
#include "gorbfs.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(storage);

// Flash Targets
static CFlightLog flight_log{"/lfs/flight.log"};
K_MSGQ_DEFINE(flightlog_msgq, sizeof(FreakFlightController::EventNotification), 4,
              alignof(FreakFlightController::EventNotification));

K_MSGQ_DEFINE(datalock_q, sizeof(bool), 1, alignof(bool));

int write_noti_to_flightlog(const FreakFlightController::EventNotification &noti) {
    static char buf[1024] = {0};
    int64_t ms = k_ticks_to_ms_near64(noti.uptimeTicks);
    if (noti.type == FreakFlightController::EventType::EventOccured) {
        LOG_INF("%lld ms: %s Occured", ms, eventNames[noti.event]);
        snprintf(buf, sizeof(buf), "%s occured", eventNames[noti.event]);
        flight_log.Write(ms, buf);
    } else if (noti.type == FreakFlightController::EventType::SourceReported) {
        snprintf(buf, sizeof(buf), "%s Reported by %s: %s", eventNames[noti.event], sourceNames[noti.source],
                 noti.hasAlreadyOccured ? "but already occured" : "");
        flight_log.Write(ms, buf);
        LOG_INF("%lld ms: %s Reported by %s: %s", ms, eventNames[noti.event], sourceNames[noti.source],
                noti.hasAlreadyOccured ? "but already occured" : "");
    }

    flight_log.Sync();
    return 0;
}

void handle_datalock_msg(DataLockMsg msg);
void lock_loop_forever();
int storage_thread_entry(void *v_fc, void *v_fdev, void *v_sdev) {
    if (is_data_locked()) {
        lock_loop_forever();
    }

    const struct device *fast_dev = *(const struct device **) v_fdev;
    const struct gorbfs_partition_config *fast_cfg = (struct gorbfs_partition_config *) fast_dev->config;
    struct gorbfs_partition_data *fast_data = (struct gorbfs_partition_data *) fast_dev->data;

    const struct device *slow_dev = *(const struct device **) v_sdev;
    const struct gorbfs_partition_config *slow_cfg = (struct gorbfs_partition_config *) slow_dev->config;
    struct gorbfs_partition_data *slow_data = (struct gorbfs_partition_data *) slow_dev->data;

    FreakFlightController *fc = static_cast<FreakFlightController *>(v_fc);
    (void) fc;

    LOG_INF("Partition %s ready for storage: Page Size: %d, Partition Size: %d, Num Pages: %d", fast_dev->name,
            PAGE_SIZE, fast_cfg->partition_size, fast_cfg->num_pages);
    LOG_INF("Partition %s ready for storage: Page Size: %d, Partition Size: %d, Num Pages: %d", slow_dev->name,
            PAGE_SIZE, slow_cfg->partition_size, slow_cfg->num_pages);

    int ret = gfs_erase_if_on_sector(fast_dev);
    if (ret != 0) {
        LOG_WRN("Failed initial erase of fast partition");
    }
    ret = gfs_erase_if_on_sector(slow_dev);
    if (ret != 0) {
        LOG_WRN("Failed initial erase of slow partition");
    }

    k_poll_event events[4];
    k_poll_event_init(&events[0], K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &datalock_q);
    k_poll_event_init(&events[1], K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, fast_data->msgq);
    k_poll_event_init(&events[2], K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, slow_data->msgq);
    k_poll_event_init(&events[3], K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &flightlog_msgq);

    while (true) {
        int ret = 0;

        ret = k_poll(events, ARRAY_SIZE(events), K_FOREVER);
        if (ret != 0) {
            LOG_WRN("Error from k poll: %d", ret);
            continue;
        }

        if (events[0].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
            DataLockMsg msg;
            ret = k_msgq_get(&datalock_q, &msg, K_FOREVER);
            if (!is_data_locked()) {
                handle_datalock_msg(msg);
            } else {
                LOG_INF("Ignoring, already locked");
            }
            events[0].state = K_POLL_STATE_NOT_READY;
        } else if (events[1].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
            void *chunk_ptr = NULL;
            ret = k_msgq_get(fast_data->msgq, &chunk_ptr, K_FOREVER);
            gfs_handle_new_block(fast_dev, chunk_ptr);
            events[1].state = K_POLL_STATE_NOT_READY;
        } else if (events[2].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
            void *chunk_ptr = NULL;
            ret = k_msgq_get(slow_data->msgq, &chunk_ptr, K_FOREVER);
            gfs_handle_new_block(slow_dev, chunk_ptr);
            events[2].state = K_POLL_STATE_NOT_READY;
        } else if (events[3].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
            FreakFlightController::EventNotification notif = {0};
            ret = k_msgq_get(&flightlog_msgq, &notif, K_FOREVER);
            events[3].state = K_POLL_STATE_NOT_READY;
            LOG_INF("Got log message %lld", notif.uptimeTicks);
            write_noti_to_flightlog(notif);
        }
    }
    if (ret != 0) {
        LOG_WRN("Couldnt write flight log: %d", ret);
    }
    ret = flight_log.Close();
    if (ret != 0) {
        LOG_WRN("Couldnt write flight log: %d", ret);
    }
    return 0;
}

void unlock_boostdata() {
    LOG_INF("Send unlock msg");
    DataLockMsg msg = DataLockMsg::Unlock;
    k_msgq_put(&datalock_q, (void *) &msg, K_FOREVER);
}
void lock_boostdata() {
    LOG_INF("Send lock msg");
    DataLockMsg msg = DataLockMsg::Lock;
    k_msgq_put(&datalock_q, (void *) &msg, K_MSEC(2));
}

static fs_file_t allowfile;
bool boostdata_locked = true;

void unlock_data_fs() {
    fs_file_t_init(&allowfile);
    int ret = fs_open(&allowfile, ALLOWFILE_PATH, FS_O_CREATE);
    if (ret != 0) {
        LOG_ERR("Couldnt create allowfile, data stays locked");
        return;
    }
    ret = fs_close(&allowfile);
    if (ret != 0) {
        LOG_ERR("Couldnt save allowfile, data (maybe) stays locked");
    }
    LOG_INF("Unlocked data successfully");
    boostdata_locked = false;
}
void lock_data_fs() {
    int ret = fs_unlink(ALLOWFILE_PATH);
    if (ret != 0) {
        LOG_ERR("Failed to delete lockfile, data stays unlocked");
        return;
    }
    LOG_INF("Locked data successfully");
    boostdata_locked = true;
}
void handle_datalock_msg(DataLockMsg toset) {
    LOG_INF("handling: %d", (int) toset);
    if (toset == DataLockMsg::Unlock) {
        unlock_data_fs();
    } else if (toset == DataLockMsg::Lock) {
        lock_data_fs();
    } else {
        LOG_ERR("Unknown boost message");
    }
}
void lock_loop_forever() {
    LOG_INF("Waiting for lock/unlock msg");

    while (true) {
        DataLockMsg msg;
        int ret = k_msgq_get(&datalock_q, &msg, K_FOREVER);
        if (ret != 0) {
            continue;
        }
        handle_datalock_msg(msg);
    }
}
