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

BUILD_ASSERT(DT_FIXED_PARTITION_EXISTS(SUPERFAST_PARTITION_NODE_ID), "I need that partition to work man");

static constexpr size_t PARTITION_ADDR = DT_REG_ADDR(SUPERFAST_PARTITION_NODE_ID);
static constexpr size_t PARTITION_SIZE = DT_REG_SIZE(SUPERFAST_PARTITION_NODE_ID);

#define BLOCK_SIZE 256
BUILD_ASSERT(sizeof(struct SuperFastPacket) == BLOCK_SIZE, "pls do that");
BUILD_ASSERT((PARTITION_SIZE % BLOCK_SIZE) == 0, "pls do that");

#define SUPER_FAST_PACKET_COUNT 5
K_MEM_SLAB_DEFINE_STATIC(superfastslab, sizeof(struct SuperFastPacket), SUPER_FAST_PACKET_COUNT,
                         alignof(struct SuperFastPacket));

K_MSGQ_DEFINE(superfastmsgq, sizeof(void *), SUPER_FAST_PACKET_COUNT, alignof(void *));

static int block_index = 0;
static constexpr int num_blocks = 100;

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
    LOG_INF("Ready for storaging");
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
        // LOG_INF("tP = %p", chunk_ptr);

        uint32_t addr = PARTITION_ADDR + block_index * BLOCK_SIZE;
        if (addr >= PARTITION_ADDR + PARTITION_SIZE) {
            LOG_WRN("Tried to write out of bounds");
            return -1;
        }
        constexpr size_t sector_size = 4096;
        if ((addr % sector_size) == 0) {
            // ret = flash_erase(flash_dev, addr, sector_size);
            // if (ret != 0) {
            // LOG_WRN("Failed to flash erase: %d", ret);
            // } else {
            // LOG_INF("Successfull flash erase\n");
            // }
        }
        ret = flash_write(flash_dev, addr, (void *) chunk_ptr, BLOCK_SIZE);
        if (ret != 0) {
            LOG_WRN("Failed to flash write at %d: %d", addr, ret);
        }
        block_index++;
        if (block_index > num_blocks) {
            block_index %= num_blocks;
        }
        k_mem_slab_free(&superfastslab, (void *) chunk_ptr);
    }
    return 0;
}

int gfs_read_block(int idx, struct SuperFastPacket *pac) {
    uint32_t addr = PARTITION_ADDR + idx * BLOCK_SIZE;
    return flash_read(flash_dev, addr, (uint8_t *) pac, 256);
}
