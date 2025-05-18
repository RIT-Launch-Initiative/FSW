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

/*
if circular, just do a circl (make sure you circle over everything tho lol)
if non circular, write finishing block if there is a block leftover
if you're the reco thingy, sucks to be you but you have gps time lol
*/

int gfs_alloc_slab(struct SuperFastPacket **slab, k_timeout_t timeout);
int gfs_submit_slab(struct SuperFastPacket *slab, k_timeout_t timeout);
int gfs_read_block(int idx, struct SuperFastPacket *slab);
int gfs_total_blocks();
int storage_thread_entry(void *, void *, void *);
