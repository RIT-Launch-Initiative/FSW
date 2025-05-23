#include "cycle_counter.hpp"
#include "data.h"
#include "flight.h"

#include <stdalign.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/fixed-partitions.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gorbfs, CONFIG_APP_FREAK_LOG_LEVEL);

int64_t cyc_waiting = 0;
int64_t cyc_erasing = 0;
int64_t cyc_writing = 0;
int64_t cyc_slabbing;

struct gorbfs_partition_config {
    const struct device *flash_dev;
    uint32_t partition_addr;
    uint32_t partition_size;
    size_t num_pages;
    bool wrap;
};

struct gorbfs_partition_data {
    size_t page_index;
    struct k_msgq *msgq;
    struct k_mem_slab *slab;
};

#ifdef CONFIG_BOARD_NATIVE_SIM
#define GORBFS_GET_FLASH_DEV(partition_name) DEVICE_DT_GET_ONE(zephyr_sim_flash)
#else
#define GORBFS_GET_FLASH_DEV(partition_name) DEVICE_DT_GET(DT_GPARENT(DT_NODE_BY_FIXED_PARTITION_LABEL(partition_name)))
#endif

#define PAGE_SIZE   256
#define SECTOR_SIZE 4096

// Must be > FLASH_INIT_PRIORITY
#define GORBFS_INIT_PRIORITY 60

BUILD_ASSERT(GORBFS_INIT_PRIORITY > CONFIG_FLASH_INIT_PRIORITY, "Gorbfs depends on flash");

static int gorbfs_init(const struct device *dev) {
    // const struct gorbfs_partition_config *config = (const struct gorbfs_partition_config *) dev->config;
    // struct gorbfs_partition_data *data = (struct gorbfs_partition_data *) dev->data;

    return 0;
}

#define GORBFS_PARTITION_DEFINE(partition_name, msg_type, buf_size)                                                           \
    BUILD_ASSERT(DT_FIXED_PARTITION_EXISTS(DT_NODE_BY_FIXED_PARTITION_LABEL(partition_name)),                          \
                 "Missing Partition for gorbfs");                                                                      \
    BUILD_ASSERT(sizeof(msg_type) == PAGE_SIZE, "Message size must be = page size (256)");                             \
    BUILD_ASSERT(DT_REG_SIZE(DT_NODE_BY_FIXED_PARTITION_LABEL(partition_name)) % PAGE_SIZE == 0,                       \
                 "Need partition size to be multiple of msg size (256)");                                              \
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
        .wrap = DT_PROP_OR(DT_NODE_BY_FIXED_PARTITION_LABEL(partition_name), wrap, false),                                    \
    };                                                                                                                 \
    struct gorbfs_partition_data gorbfs_partition_data_##partition_name{                                               \
        .page_index = 0,                                                                                               \
        .msgq = &partition_name##_msgq,                                                                                \
        .slab = &partition_name##_slab,                                                                                \
    };                                                                                                                 \
    DEVICE_DT_DEFINE(DT_NODE_BY_FIXED_PARTITION_LABEL(partition_name), gorbfs_init, NULL,                              \
                     &gorbfs_partition_data_##partition_name, &gorbfs_partition_config_##partition_name, POST_KERNEL,  \
                     GORBFS_INIT_PRIORITY, NULL);

GORBFS_PARTITION_DEFINE(superfast_storage, struct SuperFastPacket, 8);



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

int erase_on_sector(const struct gorbfs_partition_config *cfg, uint32_t page_index) {
    if (gen_addr(cfg, page_index) % SECTOR_SIZE == 0) {
        return flash_erase(cfg->flash_dev, gen_addr(cfg, page_index), SECTOR_SIZE);
    }
    return 0;
}

int storage_thread_entry(void *v_fc, void *v_dev, void *) {
    const   struct device *dev = (const struct device *)v_dev;
    const gorbfs_partition_config *cfg = (struct gorbfs_partition_config *) dev->config;
    struct gorbfs_partition_data *data = (struct gorbfs_partition_data *) dev->data;

    FreakFlightController *fc = static_cast<FreakFlightController *>(v_fc);
    (void) fc;
    if (is_boostdata_locked()) {
        LOG_WRN("Refusing to start storage thread bc data is locked");
        return -ENOTSUP;
    }
    LOG_INF("Ready for storaging: Page Size: %d, Partition Size: %d, Num Pages: %d", PAGE_SIZE, cfg->partition_size,
            cfg->num_pages);

    while (true) {
        int ret = 0;
        size_t addr = gen_addr(cfg, data->page_index);
        {
            CycleCounter _(cyc_erasing);
            ret = erase_on_sector(cfg, data->page_index);
        }

        SuperFastPacket *chunk_ptr = NULL;
        {
            CycleCounter _(cyc_waiting);
            ret = k_msgq_get(data->msgq, &chunk_ptr, K_FOREVER);
        }
        if (ret != 0) {
            LOG_WRN("Wait on super fast msgq completed with error %d", ret);
            continue;
        }
        if (chunk_ptr == NULL) {
            LOG_WRN("Received NULL from msgq");
            continue;
        }

        if (addr >= cfg->partition_addr + cfg->partition_size) {
            LOG_WRN("Tried to write out of bounds: End: %d, addr: %d, ind: %d",
                    (cfg->partition_addr + cfg->partition_size), addr, data->page_index);
            return -1;
        }
        {
            CycleCounter _(cyc_writing);
            ret = flash_write(cfg->flash_dev, addr, (void *) chunk_ptr, PAGE_SIZE);
        }
        if (ret != 0) {
            LOG_WRN("Failed to flash write at %d: %d", addr, ret);
        }
        data->page_index++;
        if (data->page_index >= cfg->num_pages) {
            data->page_index %= cfg->num_pages;
        }
        {
            CycleCounter _(cyc_slabbing);
            k_mem_slab_free(data->slab, (void *) chunk_ptr);
        }
    }
    return 0;
}

int gfs_total_blocks(device *dev) {
    const gorbfs_partition_config *cfg = (struct gorbfs_partition_config *) dev->config;
    // struct gorbfs_partition_data *data = (struct gorbfs_partition_data *) dev->data;

    return cfg->partition_size / PAGE_SIZE;
}

int gfs_read_block(device *dev, int idx, struct SuperFastPacket *pac) {
    const gorbfs_partition_config *cfg = (struct gorbfs_partition_config *) dev->config;
    // struct gorbfs_partition_data *data = (struct gorbfs_partition_data *) dev->data;

    uint32_t addr = cfg->partition_addr + idx * PAGE_SIZE;
    return flash_read(cfg->flash_dev, addr, (uint8_t *) pac, PAGE_SIZE);
}

