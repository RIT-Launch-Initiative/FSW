#include "common.h"
#include "f_core/os/flight_log.hpp"
#include "flight.h"

#include <stdalign.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/fixed-partitions.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gorbfs, CONFIG_APP_FREAK_LOG_LEVEL);

struct gorbfs_partition_config {
    // flash to write to
    const struct device *flash_dev;
    // start of partition
    uint32_t partition_addr;
    // size of partition
    uint32_t partition_size;
    // size of partition in pages
    size_t num_pages;
    // length back in time to save
    // if i have circle_size = 20, when i signal end of circle, save 20 pages back
    uint32_t circle_size_pages;
};

struct gorbfs_partition_data {
    // current index into flash that we're writing at
    size_t page_index;
    // msgq used as a way to pass slabs in a thread safe way
    struct k_msgq *msgq;
    // underlying slab used to hold data
    struct k_mem_slab *slab;
    // page index that the data to keep starts or UNSET_START_INDEX while operating in circle mode
    uint32_t start_index;
};

#ifdef CONFIG_BOARD_NATIVE_SIM
#define GORBFS_GET_FLASH_DEV(partition_name) DEVICE_DT_GET_ONE(zephyr_sim_flash)
#else
#define GORBFS_GET_FLASH_DEV(partition_name) DEVICE_DT_GET(DT_GPARENT(DT_NODE_BY_FIXED_PARTITION_LABEL(partition_name)))
#endif

#define PAGE_SIZE   256
#define SECTOR_SIZE 4096

#define GORBFS_INIT_PRIORITY 60
BUILD_ASSERT(GORBFS_INIT_PRIORITY > CONFIG_FLASH_INIT_PRIORITY, "Gorbfs depends on flash");

static int gorbfs_init(const struct device *dev) { return 0; }

#define UNSET_START_INDEX UINT32_MAX
#define SET_START_INDEX   (UINT32_MAX - 1)

#define GORBFS_PARTITION_DEFINE(partition_name, msg_type, buf_size, circ_size)                                         \
    BUILD_ASSERT(DT_FIXED_PARTITION_EXISTS(DT_NODE_BY_FIXED_PARTITION_LABEL(partition_name)),                          \
                 "Missing Partition for gorbfs");                                                                      \
    BUILD_ASSERT(sizeof(msg_type) == PAGE_SIZE, "Message size must be = page size (256)");                             \
    BUILD_ASSERT(DT_REG_SIZE(DT_NODE_BY_FIXED_PARTITION_LABEL(partition_name)) % PAGE_SIZE == 0,                       \
                 "Need partition size to be multiple of msg size (256)");                                              \
    BUILD_ASSERT(                                                                                                      \
        DT_REG_SIZE(DT_NODE_BY_FIXED_PARTITION_LABEL(partition_name)) % SECTOR_SIZE == 0,                              \
        "Need partition size to be multiple of sector size (4096) or we'll explode the next partition with an erase"); \
    BUILD_ASSERT(DT_REG_ADDR(DT_NODE_BY_FIXED_PARTITION_LABEL(partition_name)) % SECTOR_SIZE == 0,                     \
                 "Need partition addr mod sector_size = 0 to be able to do aligned erases");                           \
                                                                                                                       \
    K_MEM_SLAB_DEFINE_STATIC(partition_name##_slab, sizeof(msg_type), buf_size, alignof(msg_type));                    \
                                                                                                                       \
    K_MSGQ_DEFINE(partition_name##_msgq, sizeof(void *), buf_size, alignof(void *));                                   \
                                                                                                                       \
    const struct gorbfs_partition_config gorbfs_partition_config_##partition_name{                                     \
        .flash_dev = GORBFS_GET_FLASH_DEV(partition_name),                                                             \
        .partition_addr = DT_REG_ADDR(DT_NODE_BY_FIXED_PARTITION_LABEL(partition_name)),                               \
        .partition_size = DT_REG_SIZE(DT_NODE_BY_FIXED_PARTITION_LABEL(partition_name)),                               \
        .num_pages = DT_REG_SIZE(DT_NODE_BY_FIXED_PARTITION_LABEL(partition_name)) / PAGE_SIZE,                        \
        .circle_size_pages = circ_size,                                                                                \
    };                                                                                                                 \
    struct gorbfs_partition_data gorbfs_partition_data_##partition_name{                                               \
        .page_index = 0,                                                                                               \
        .msgq = &partition_name##_msgq,                                                                                \
        .slab = &partition_name##_slab,                                                                                \
        .start_index = UNSET_START_INDEX,                                                                              \
    };                                                                                                                 \
    DEVICE_DT_DEFINE(DT_NODE_BY_FIXED_PARTITION_LABEL(partition_name), gorbfs_init, NULL,                              \
                     &gorbfs_partition_data_##partition_name, &gorbfs_partition_config_##partition_name, POST_KERNEL,  \
                     GORBFS_INIT_PRIORITY, NULL);

// Flash Targets
static CFlightLog flight_log{"/lfs/flight.log"};

GORBFS_PARTITION_DEFINE(superfast_storage, NTypes::SuperFastPacket, 8, 500);
GORBFS_PARTITION_DEFINE(superslow_storage, SuperSlowPacket, 2, 6);

// helper to create an address from an index
constexpr uint32_t gen_addr(const gorbfs_partition_config *cfg, uint32_t page_index) {
    return cfg->partition_addr + page_index * PAGE_SIZE;
}

int gfs_alloc_slab(const struct device *dev, void **slab_ptr, k_timeout_t timeout) {
    struct gorbfs_partition_data *data = (struct gorbfs_partition_data *) dev->data;
    int ret = k_mem_slab_alloc(data->slab, slab_ptr, timeout);
    return ret;
}
int gfs_submit_slab(const struct device *dev, void *slab, k_timeout_t timeout) {
    struct gorbfs_partition_data *data = (struct gorbfs_partition_data *) dev->data;
    int ret = k_msgq_put(data->msgq, &slab, timeout);
    return ret;
}

int erase_if_on_sector(const struct device *gfs_dev) {
    const gorbfs_partition_config *cfg = (struct gorbfs_partition_config *) gfs_dev->config;
    struct gorbfs_partition_data *data = (struct gorbfs_partition_data *) gfs_dev->data;

    if (gen_addr(cfg, data->page_index) % SECTOR_SIZE == 0) {
        return flash_erase(cfg->flash_dev, gen_addr(cfg, data->page_index), SECTOR_SIZE);
    }
    return 0;
}

int set_cutoff_if_needed(const struct device *gfs_dev) {
    const gorbfs_partition_config *cfg = (struct gorbfs_partition_config *) gfs_dev->config;
    struct gorbfs_partition_data *data = (struct gorbfs_partition_data *) gfs_dev->data;

    uint32_t cutoff_index = data->start_index;
    if (cutoff_index == SET_START_INDEX) {
        if (data->page_index >= cfg->circle_size_pages) {
            cutoff_index = data->page_index - cfg->circle_size_pages;
        } else {
            cutoff_index = data->page_index + cfg->num_pages - cfg->circle_size_pages;
        }
        data->start_index = cutoff_index;
        LOG_INF("Setting start index to %d (was at %d with circ size %d)", cutoff_index, data->page_index,
                cfg->circle_size_pages);
        int start = k_uptime_get();
        flight_log.Write("yeah man im swapping its crazy");
        int end = k_uptime_get() - start;
        LOG_INF("Wrote in %d", end);
    }
    return 0;
}

int handle_new_block(const struct device *gfs_dev, void *chunk_ptr) {
    const gorbfs_partition_config *cfg = (struct gorbfs_partition_config *) gfs_dev->config;
    struct gorbfs_partition_data *data = (struct gorbfs_partition_data *) gfs_dev->data;

    if (chunk_ptr == NULL) {
        LOG_WRN("Received NULL from msgq");
        return -ENODATA;
    }
    int ret = 0;

    // If the end of the circle has been marked, set it
    set_cutoff_if_needed(gfs_dev);

    size_t addr = gen_addr(cfg, data->page_index);

    if (data->page_index == data->start_index) {
        LOG_WRN_ONCE("Discarding page because flash is saturated");
        k_mem_slab_free(data->slab, (void *) chunk_ptr);
        return -ENOSPC;
    }

    if (addr >= cfg->partition_addr + cfg->partition_size) {
        LOG_WRN("Tried to write out of bounds: End: %d, addr: %d, ind: %d", (cfg->partition_addr + cfg->partition_size),
                addr, data->page_index);
        k_mem_slab_free(data->slab, (void *) chunk_ptr);
        return -EOVERFLOW;
    }
    ret = flash_write(cfg->flash_dev, addr, (void *) chunk_ptr, PAGE_SIZE);
    if (ret != 0) {
        LOG_WRN("Failed to flash write at %d: %d", addr, ret);
    }
    data->page_index++;
    if (data->page_index >= cfg->num_pages) {
        data->page_index %= cfg->num_pages;
    }
    k_mem_slab_free(data->slab, (void *) chunk_ptr);

    // Erase if next block will go to new sector
    return erase_if_on_sector(gfs_dev);
}

K_MSGQ_DEFINE(datalock_q, sizeof(bool), 1, alignof(bool));
void handle_datalock_msg(DataLockMsg msg);
void lock_loop_forever();
int storage_thread_entry(void *v_fc, void *v_fdev, void *v_sdev) {
    if (is_data_locked()) {
        lock_loop_forever();
    }

    const struct device *fast_dev = *(const struct device **) v_fdev;
    const gorbfs_partition_config *fast_cfg = (struct gorbfs_partition_config *) fast_dev->config;
    struct gorbfs_partition_data *fast_data = (struct gorbfs_partition_data *) fast_dev->data;

    const struct device *slow_dev = *(const struct device **) v_sdev;
    const gorbfs_partition_config *slow_cfg = (struct gorbfs_partition_config *) slow_dev->config;
    struct gorbfs_partition_data *slow_data = (struct gorbfs_partition_data *) slow_dev->data;

    FreakFlightController *fc = static_cast<FreakFlightController *>(v_fc);
    (void) fc;

    LOG_INF("Partition %s ready for storage: Page Size: %d, Partition Size: %d, Num Pages: %d", fast_dev->name,
            PAGE_SIZE, fast_cfg->partition_size, fast_cfg->num_pages);
    LOG_INF("Partition %s ready for storage: Page Size: %d, Partition Size: %d, Num Pages: %d", slow_dev->name,
            PAGE_SIZE, slow_cfg->partition_size, slow_cfg->num_pages);

    int ret = erase_if_on_sector(fast_dev);
    if (ret != 0) {
        LOG_WRN("Failed initial erase of fast partition");
    }
    ret = erase_if_on_sector(slow_dev);
    if (ret != 0) {
        LOG_WRN("Failed initial erase of slow partition");
    }

    k_poll_event events[3];
    k_poll_event_init(&events[0], K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &datalock_q);
    k_poll_event_init(&events[1], K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, fast_data->msgq);
    k_poll_event_init(&events[2], K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, slow_data->msgq);

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
            handle_new_block(fast_dev, chunk_ptr);
            events[1].state = K_POLL_STATE_NOT_READY;
        } else if (events[2].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
            void *chunk_ptr = NULL;
            ret = k_msgq_get(slow_data->msgq, &chunk_ptr, K_FOREVER);
            handle_new_block(slow_dev, chunk_ptr);
            events[2].state = K_POLL_STATE_NOT_READY;
        }
    }
    ret = flight_log.Write("Yeah man im done wild huh");
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

int gfs_total_blocks(const struct device *dev) {
    const gorbfs_partition_config *cfg = (struct gorbfs_partition_config *) dev->config;

    return cfg->partition_size / PAGE_SIZE;
}

int gfs_read_block(const struct device *dev, int idx, uint8_t *pac) {
    const gorbfs_partition_config *cfg = (struct gorbfs_partition_config *) dev->config;

    uint32_t addr = cfg->partition_addr + idx * PAGE_SIZE;
    return flash_read(cfg->flash_dev, addr, pac, PAGE_SIZE);
}

int gfs_signal_end_of_circle(const struct device *dev) {
    struct gorbfs_partition_data *data = (struct gorbfs_partition_data *) dev->data;
    if (data->start_index != UNSET_START_INDEX) {
        LOG_WRN_ONCE("Attempting to set start index that was already set, ignoring");
        return -ENOTSUP;
    }
    data->start_index = SET_START_INDEX;
    return 0;
}