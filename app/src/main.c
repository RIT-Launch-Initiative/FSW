#include "fs_utils.h"

#include <zephyr/kernel.h>
#include <app_version.h>
#include <zephyr/device.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/sensor/tmp116.h>

#include <zephyr/logging/log.h>

#include <zephyr/net/socket.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>


#define SLEEP_TIME_MS   1000

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

#if DT_NODE_EXISTS(PARTITION_NODE)
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
#else /* PARTITION_NODE */

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t lfs_storage_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &storage,
	.storage_dev = (void *) FIXED_PARTITION_ID(storage_partition),
	.mnt_point = "/lfs",
};
#endif /* PARTITION_NODE */

struct fs_mount_t *mount_point = &lfs_storage_mnt;


static int init() {
    int ret = init_fs(mount_point);
    
    // TODO: Initialize other stuff we want to here
    return ret;
}


int main(void) {
    int ret = init();
    

	return 0;
}
