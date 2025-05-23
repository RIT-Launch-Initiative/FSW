#include "data.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

int storage_thread_entry(void *, void *, void *);

int gfs_alloc_slab(const struct device *dev, void **slab_ptr, k_timeout_t timeout);
int gfs_submit_slab(const struct device *dev, void *slab, k_timeout_t timeout);
int gfs_total_blocks(const struct device *dev);
int gfs_read_block(const struct device *dev, int idx, struct SuperFastPacket *pac);