#pragma once
/**
 * Brain dead simple storage system for time series data on flash devices
 * Doesn't even index your partitions, but you're adding a timestamp or index bc its time series data anyways
 * The only interesting feature is the circular buffer mode.
 * 
 * General principle of operation is that you can use the partition as a flash backed circular buffer such that you can record data before you know you need to save it (boost detection)
 * If that data remains unused, no need to save it and it will be overwritten.
 * Once you know you need to save that data, gfs_signal_end_of_circle can be used to tell the partition to stop overwriting data as of N pages ago where N is a preconfigured number selected depending on your detection delay
 * If you don't need this wrapping behavior, call gfs_signal_end_of_circle on init with a circle size of 0 to never overwrite
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

/**
 * Get a buffer to write into
 * This should *always* be submitted back to the storage system with gfs_submit_slab
 * @param dev the device gotten by DEVICE_DT_GET identifying this gfs partition
 * @param slab_ptr pointer to the pointer that will identify the buffer to write into
 * @param timeout timeout for waiting for a slab to be ready to be allocate
 * @retval 0 Memory allocated. The block address area pointed at by slab_ptr
 *         is set to the starting address of the memory block.
 * @retval -ENOBUFS The file is already saturated, so not allocating for you
 * @retval -ENOMEM Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EINVAL Invalid data supplied
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
 * Free a slab without submitting it to be stored
 * only call if slab not already submitted (submit_slab returns non zero)
 */
void gfs_free_slab(const struct device *dev, void *slab);

/**
 * Get the total number of blocks (pages) that the partition holds
 * @param dev the device gotten by DEVICE_DT_GET identifying this gfs partition
 * @return the number of blocks this partition holds
 */
int gfs_total_blocks(const struct device *dev);

/**
 * Check if the circular buffer has had its stopping point set
 * @return true if already set, false if not
 */
bool gfs_circle_point_set(const struct device *dev);

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
 * gfs_handle_poll_item calls this if you don't want to wait on individual msgqs all the time
 */
int gfs_handle_new_block(const struct device *gfs_dev, void *chunk_ptr);

/**
 * Erase the block if the block needs to be erase
 * exposed for initial init
 * not done at init time in case you need to check something before you erase a sector
 * theres a good chance you dont need to call this manually
 */
int gfs_erase_if_on_sector(const struct device *gfs_dev);

/**
 * Check if the partition is already saturated
 * @param dev the device gotten by DEVICE_DT_GET identifying this gfs partition
 * @returns true if the partition has been marked as saturated
 */
bool gfs_partition_saturated(const struct device *dev);

// initialize a poll item for this partition
// https://docs.zephyrproject.org/latest/kernel/services/polling.html
void gfs_poll_item_init(const struct device *gfs_dev, struct k_poll_event *event);
/**
 * Handle a submitted block as received by k_poll
 * This function *WILL* reset event.state to K_POLL_STATE_NOT_READY so the user does not need to do that manually.
 */
int gfs_handle_poll_item(const struct device *gfs_dev, struct k_poll_event *event);

#define GFS_POLL_ITEM_AVAILABLE(event) (event.state == K_POLL_STATE_MSGQ_DATA_AVAILABLE)
