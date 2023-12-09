#include "fs_utils.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int init_fs(struct fs_mount_t *mount_point) {
    int ret = fs_wipe((uintptr_t) mount_point->storage_dev);
	if (ret < 0) {
		return ret;
	}

	/* Do not mount if auto-mount has been enabled */
#if !DT_NODE_EXISTS(PARTITION_NODE) || !(FSTAB_ENTRY_DT_MOUNT_FLAGS(PARTITION_NODE) & FS_MOUNT_FLAG_AUTOMOUNT)
    
	ret = fs_mount(mount_point);
	
    if (ret < 0) {
		LOG_PRINTK("FAIL: mount id %" PRIuPTR " at %s: %d\n", (uintptr_t) mount_point->storage_dev, mount_point->mnt_point, ret);
		return ret;
	}

	LOG_PRINTK("%s mount: %d\n", mount_point->mnt_point, ret);
#else
	LOG_PRINTK("%s automounted\n", mount_point->mnt_point);
#endif


    return 0;
}


int fs_wipe(uint32_t id) {
	const struct flash_area *pfa;
	int ret = flash_area_open(id, &pfa);

	if (ret < 0) {
		LOG_ERR("FAIL: unable to find flash area %u: %d\n", id, ret);
		return ret;
	}

	LOG_PRINTK("Area %u at 0x%x on %s for %u bytes\n",
		   id, (unsigned int)pfa->fa_off, pfa->fa_dev->name,
		   (unsigned int)pfa->fa_size);

	/* Optional wipe flash contents */
	if (IS_ENABLED(CONFIG_APP_WIPE_STORAGE)) {
		ret = flash_area_erase(pfa, 0, pfa->fa_size);
		LOG_ERR("Erasing flash area ... %d", ret);
	}

	flash_area_close(pfa);
	return ret;
}


int fs_list(const char * path) {

    return 0;
}
