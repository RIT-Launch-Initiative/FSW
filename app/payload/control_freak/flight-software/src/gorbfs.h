#include "data.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>

int gfs_alloc_slab(struct SuperFastPacket **slab, k_timeout_t timeout);
int gfs_submit_slab(struct SuperFastPacket *slab, k_timeout_t timeout);
int gfs_read_block(int idx, struct SuperFastPacket *slab);
int gfs_total_blocks();
int storage_thread_entry(void *, void *, void *);
