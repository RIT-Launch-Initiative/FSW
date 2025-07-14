# Gorb FS

storage system for circular buffering time series
for block devices

### Setup

use dt tags to setup partitions
use device_dt_get in both producer and consumer to get handle to that partition

### Producers
- allocate blocks using `gfs_alloc_slab`
- fill with whatever data
- submit with `gfs_submit_slab`
- mark the end of a circle using `gfs_signal_end_of_circle`

### Consumers
- listen for data with gfs_wait_for_event()
