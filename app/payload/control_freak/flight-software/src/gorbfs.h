#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

// Brain dead fast time series file storage
// follows ideas from zephyr i2s subsystem using k_mem_slab to minimize copies

/**
 * Get a buffer to write into
 * This should *always* be submitted back to the storage system with gfs_submit_slab
 * @param dev the device gotten by DEVICE_DT_GET identifying this gfs partition
 * @param slab_ptr pointer to the pointer that will identify the buffer to write into
 * @param timeout timeout for waiting for a slab to be ready to be allocate
 */
int gfs_alloc_slab(const struct device *dev, void **slab_ptr, k_timeout_t timeout);
/**
 * Submit a buffer to be processed
 * @param dev the device gotten by DEVICE_DT_GET identifying this gfs partition
 * @param slab the slab (as received by gfs_alloc_slab) containing the data to write
 * @param timeout the time to wait for submitting the slab (more time = more time for flash to catch up)
 */
int gfs_submit_slab(const struct device *dev, void *slab, k_timeout_t timeout);
/**
 * Get the total number of blocks (pages) that the partition holds
 * @param dev the device gotten by DEVICE_DT_GET identifying this gfs partition
 * @return the number of blocks this partition holds
 */
int gfs_total_blocks(const struct device *dev);
/**
 * Tell the system that it should stop circular buffer mode and start saturating mode
 * when this is called, the system marks CURRENT_PAGE - CIRCLE_SIZE_PAGES as the point to stop
 * @param dev the device gotten by DEVICE_DT_GET identifying this gfs partition
 * @return error code 
 */
int gfs_signal_end_of_circle(const struct device *dev);

/**
 * Read a block straight from flash based off it gfs index
 * @param dev the device gotten by DEVICE_DT_GET identifying this gfs partition
 * @param idx the page index to read from
 * @param pac the buffer to read into (must be 256 bytes or greater)
 * This function always reads 256 bytes
 * @return 0 on success, negative error code on flash read failure
 */
int gfs_read_block(const struct device *dev, int idx, uint8_t *pac);

/**
 * Actual flash actioner
 * call on you flash thread if your flash driver isnt thread safe o_O
 */
int gfs_handle_new_block(const struct device *gfs_dev, void *chunk_ptr);

/**
 * Erase the block if the block needs to be erase
 * exposed for initial init
 * not done at init time in case you need to check something before you erase a sector
 */
int gfs_erase_if_on_sector(const struct device *gfs_dev);

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
