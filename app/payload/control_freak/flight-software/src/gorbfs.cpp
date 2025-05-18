#include "data.h"
#include "flight.h"

#include <stdalign.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/fixed-partitions.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(storage, CONFIG_APP_FREAK_LOG_LEVEL);

bool is_boostdata_locked() { return false; }

#define SUPERFAST_PARTITION_NODE_ID DT_NODE_BY_FIXED_PARTITION_LABEL(superfast_storage)

#ifdef CONFIG_BOARD_NATIVE_SIM
const struct device *flash_dev = DEVICE_DT_GET_ONE(zephyr_sim_flash);
#else
#define FLASH_DEV                   DT_GPARENT(SUPERFAST_PARTITION_NODE_ID)
const struct device *flash_dev = DEVICE_DT_GET(FLASH_DEV);
#endif

#define ERASE_SIZE 4096
BUILD_ASSERT(DT_FIXED_PARTITION_EXISTS(SUPERFAST_PARTITION_NODE_ID), "I need that partition to work man");
static constexpr size_t PARTITION_ADDR = DT_REG_ADDR(SUPERFAST_PARTITION_NODE_ID);
static constexpr size_t PARTITION_SIZE = DT_REG_SIZE(SUPERFAST_PARTITION_NODE_ID);
BUILD_ASSERT(PARTITION_ADDR % ERASE_SIZE == 0, "Need to be able to do aligned erases");

#define PAGE_SIZE 256
BUILD_ASSERT(sizeof(struct SuperFastPacket) == PAGE_SIZE, "pls do that");
BUILD_ASSERT((PARTITION_SIZE % PAGE_SIZE) == 0, "pls do that");

#define SUPER_FAST_PACKET_COUNT 8
K_MEM_SLAB_DEFINE_STATIC(superfastslab, sizeof(struct SuperFastPacket), SUPER_FAST_PACKET_COUNT,
                         alignof(struct SuperFastPacket));

K_MSGQ_DEFINE(superfastmsgq, sizeof(void *), SUPER_FAST_PACKET_COUNT, alignof(void *));

static int page_index = 0;
static constexpr int num_pages = PARTITION_SIZE / PAGE_SIZE;

K_MUTEX_DEFINE(mut);

int gfs_alloc_slab(struct SuperFastPacket **slab, k_timeout_t timeout) {
    if (is_boostdata_locked()) {
        return -ENOMEM;
    }
    int ret = k_mem_slab_alloc(&superfastslab, (void **) slab, timeout);
    return ret;
}
int gfs_submit_slab(struct SuperFastPacket *slab, k_timeout_t timeout) {
    if (is_boostdata_locked()) {
        return -EACCES;
    }
    int ret = k_msgq_put(&superfastmsgq, (void *) &slab, timeout);
    return ret;
}

int storage_thread_entry(void *v_fc, void *, void *) {
    FreakFlightController *fc = static_cast<FreakFlightController *>(v_fc);
    (void) fc;
    if (is_boostdata_locked()) {
        LOG_WRN("Refusing to start storage thread bc data is locked");
    }
    LOG_INF("Ready for storaging: Page Size: %d, Partition Size: %d, Num Pages: %d", PAGE_SIZE, PARTITION_SIZE,
            num_pages);

    size_t next_addr = PARTITION_ADDR;
    constexpr size_t sector_size = 4096;
    if ((next_addr % sector_size) == 0) {
        int ret = flash_erase(flash_dev, next_addr, sector_size);
        if (ret != 0) {
            LOG_WRN("Failed to flash erase: %d", ret);
        } else {
            // LOG_INF("Successfull flash erase\n");
        }
    }

    while (true) {
        SuperFastPacket *chunk_ptr = NULL;
        int ret = k_msgq_get(&superfastmsgq, &chunk_ptr, K_FOREVER);
        if (ret != 0) {
            LOG_WRN("Wait on super fast msgq completed with error %d", ret);
            continue;
        }
        if (chunk_ptr == NULL) {
            LOG_WRN("Received NULL from msgq");
            continue;
        }

        uint32_t addr = PARTITION_ADDR + page_index * PAGE_SIZE;
        if (addr >= PARTITION_ADDR + PARTITION_SIZE) {
            LOG_WRN("Tried to write out of bounds: End: %d, addr: %d, ind: %d", (PARTITION_ADDR + PARTITION_SIZE), addr,
                    page_index);
            return -1;
        }
        ret = flash_write(flash_dev, addr, (void *) chunk_ptr, PAGE_SIZE);
        if (ret != 0) {
            LOG_WRN("Failed to flash write at %d: %d", addr, ret);
        }
        page_index++;
        if (page_index >= num_pages) {
            page_index %= num_pages;
        }
        k_mem_slab_free(&superfastslab, (void *) chunk_ptr);

        size_t next_addr = PARTITION_ADDR + page_index * PAGE_SIZE;
        if ((next_addr % sector_size) == 0) {
            int ret = flash_erase(flash_dev, next_addr, sector_size);
            if (ret != 0) {
                LOG_WRN("Failed to flash erase: %d", ret);
            } else {
                // LOG_INF("Successfull flash erase\n");
            }
        }
    }
    return 0;
}

int gfs_total_blocks() { return PARTITION_SIZE / PAGE_SIZE; }

int gfs_read_block(int idx, struct SuperFastPacket *pac) {
    uint32_t addr = PARTITION_ADDR + idx * PAGE_SIZE;
    return flash_read(flash_dev, addr, (uint8_t *) pac, 256);
}
