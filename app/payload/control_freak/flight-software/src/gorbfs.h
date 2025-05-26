
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

// Brain dead fast time series file storage
// follows ideas from zephyr i2s subsystem using k_mem_slab to minimize copies
//
int storage_thread_entry(void *, void *, void *);

int gfs_alloc_slab(const struct device *dev, void **slab_ptr, k_timeout_t timeout);
int gfs_submit_slab(const struct device *dev, void *slab, k_timeout_t timeout);
int gfs_total_blocks(const struct device *dev);
// Tell the system that it should stop circular buffer mode and start saturating mode
// when this is called, the system marks CURRENT_PAGE - CIRCLE_SIZE_PAGES as the point to stop
int gfs_signal_end_of_circle(const struct device *dev);
int gfs_read_block(const struct device *dev, int idx, uint8_t *pac);
