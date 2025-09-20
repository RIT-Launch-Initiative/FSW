#include "gorbfs.h"

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

#ifdef CONFIG_BOARD_NATIVE_SIM
#define GORBFS_GET_FLASH_DEV(partition_name) DEVICE_DT_GET_ONE(zephyr_sim_flash)
#else
#define GORBFS_GET_FLASH_DEV(partition_name) DEVICE_DT_GET(DT_GPARENT(DT_NODE_BY_FIXED_PARTITION_LABEL(partition_name)))
#endif

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

GORBFS_PARTITION_DEFINE(superfast_storage, NTypes::SuperFastPacket, 8, 500);
GORBFS_PARTITION_DEFINE(superslow_storage, SuperSlowPacket, 2, 6);
GORBFS_PARTITION_DEFINE(superyev_storage, YevPacket, 4, 32);

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

int gfs_erase_if_on_sector(const struct device *gfs_dev) {
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
        int end = k_uptime_get() - start;
        LOG_INF("Wrote in %d", end);
    }
    return 0;
}

int gfs_handle_new_block(const struct device *gfs_dev, void *chunk_ptr) {
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
    return gfs_erase_if_on_sector(gfs_dev);
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