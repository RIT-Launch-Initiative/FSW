#include "data.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>
// Parts
// Memslab
// Message Queue for submitting slab chunks
//

// Storage Thread
// loop {
//     wait for data
//     write to flash
//     free slab
// }
//
// feeding thread {
//    get slab
//    do stuff
//    submit slab
// }

int gfs_alloc_slab(struct SuperFastPacket **slab, k_timeout_t timeout);
int gfs_submit_slab(struct SuperFastPacket *slab, k_timeout_t timeout);

int storage_thread_entry(void *, void *, void *);