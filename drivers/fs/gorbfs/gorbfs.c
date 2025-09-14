#include "gorbfs.h"

#include <stdalign.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/fixed-partitions.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT zephyr_fstab_gorbfs

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

#define PAGE_SIZE   256
#define SECTOR_SIZE 4096

LOG_MODULE_REGISTER(gorbfs, CONFIG_APP_AIRBRAKE_LOG_LEVEL);

#ifdef CONFIG_BOARD_NATIVE_SIM
#define GORBFS_GET_FLASH_DEV(node) DEVICE_DT_GET_ONE(zephyr_sim_flash)
#else
#define GORBFS_GET_FLASH_DEV(node) DEVICE_DT_GET(DT_GPARENT(node))
#endif

#define GORBFS_INIT_PRIORITY 60
BUILD_ASSERT(GORBFS_INIT_PRIORITY > CONFIG_FLASH_INIT_PRIORITY, "Gorbfs depends on flash");

static int gorbfs_init(const struct device *dev) { return 0; }

#define UNSET_START_INDEX UINT32_MAX
#define SET_START_INDEX   (UINT32_MAX - 1)

// helper to create an address from an index
uint32_t gen_addr(const struct gorbfs_partition_config *cfg, uint32_t page_index) {
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
    const struct gorbfs_partition_config *cfg = (struct gorbfs_partition_config *) gfs_dev->config;
    struct gorbfs_partition_data *data = (struct gorbfs_partition_data *) gfs_dev->data;

    if (gen_addr(cfg, data->page_index) % SECTOR_SIZE == 0) {
        return flash_erase(cfg->flash_dev, gen_addr(cfg, data->page_index), SECTOR_SIZE);
    }
    return 0;
}

int set_cutoff_if_needed(const struct device *gfs_dev) {
    const struct gorbfs_partition_config *cfg = (struct gorbfs_partition_config *) gfs_dev->config;
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
    const struct gorbfs_partition_config *cfg = (struct gorbfs_partition_config *) gfs_dev->config;
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
    const struct gorbfs_partition_config *cfg = (struct gorbfs_partition_config *) dev->config;

    return cfg->partition_size / PAGE_SIZE;
}

int gfs_read_block(const struct device *dev, int idx, uint8_t *pac) {
    const struct gorbfs_partition_config *cfg = (struct gorbfs_partition_config *) dev->config;

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

#define GORBFS_PARTITION_DEFINE(n, node)                                                   \
    BUILD_ASSERT(DT_REG_SIZE(DT_PROP(node, partition)) % PAGE_SIZE == 0,                       \
                 "Need partition size to be multiple of msg size (256)");                                              \
    BUILD_ASSERT(                                                                                                      \
        DT_REG_SIZE(DT_PROP(node, partition)) % SECTOR_SIZE == 0,                              \
        "Need partition size to be multiple of sector size (4096) or we'll explode the next partition with an erase"); \
    BUILD_ASSERT(DT_REG_ADDR(DT_PROP(node, partition)) % SECTOR_SIZE == 0,                     \
                 "Need partition addr mod sector_size = 0 to be able to do aligned erases");                           \
                                                                                                                       \
    K_MEM_SLAB_DEFINE_STATIC(partition_slab##n, PAGE_SIZE, DT_PROP(node, ram_cache_blocks), alignof(uint8_t[256]));                    \
                                                                                                                       \
    K_MSGQ_DEFINE(partition_msgq##n, sizeof(void *), DT_PROP(node, ram_cache_blocks), alignof(void *));                                   \
                                                                                                                       \
    const struct gorbfs_partition_config gorbfs_partition_config_##n = {                                     \
        .flash_dev = GORBFS_GET_FLASH_DEV(node),                                                             \
        .partition_addr = DT_REG_ADDR(DT_PROP(node, partition)),                               \
        .partition_size = DT_REG_SIZE(DT_PROP(node, partition)),                               \
        .num_pages = DT_REG_SIZE(DT_PROP(node, partition)) / PAGE_SIZE,                        \
        .circle_size_pages = DT_PROP(node, circle_size),                                                                                \
    };                                                                                                                 \
    struct gorbfs_partition_data gorbfs_partition_data_##n = {                                               \
        .page_index = 0,                                                                                               \
        .msgq = &partition_msgq##n,                                                                                \
        .slab = &partition_slab##n,                                                                                \
        .start_index = UNSET_START_INDEX,                                                                              \
    };  \   
    DEVICE_DT_DEFINE(node, gorbfs_init, NULL,                              \
                     &gorbfs_partition_data_##n, &gorbfs_partition_config_##n, POST_KERNEL,  \
                     GORBFS_INIT_PRIORITY, NULL);
#warning "asda"
#define GFS_PART_INIT(n)                                                                                               \
    GORBFS_PARTITION_DEFINE(n, DT_INST(n, DT_DRV_COMPAT))

DT_INST_FOREACH_STATUS_OKAY(GFS_PART_INIT)