#include "zephyr/fs/fs_interface.h"
#include "zephyr/logging/log_core.h"
#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sample, LOG_LEVEL_DBG);

// node identifiers
// NOTE: Hardware prerequisites
// 	Launch Mikroe Click Shield
// 	W25Q128JV breakout connected to port 1

// devicetree gets
#define LED_NODE DT_ALIAS(led)
#define FLASH_NODE DT_ALIAS(storage)

// create mount point structure from devicetree
#define LFS2 DT_NODELABEL(lfs2)
// NOTE: The sample uses "PARTITION_NODE" to refer to the fstab node, not the
// partition itself
FS_FSTAB_DECLARE_ENTRY(LFS2);
struct fs_mount_t *lfs2_mnt_p = &FS_FSTAB_ENTRY(LFS2);

// create mount point structure from scratch
FS_LITTLEFS_DECLARE_CUSTOM_CONFIG(lfs3, 4, 16, 16, 64, 32);
// FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(lfs3);
static struct fs_mount_t lfs3_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &lfs3,
	.storage_dev = (void*) FIXED_PARTITION_ID(ext_storage_3),
	.mnt_point = "/lfs3"
};

// create peripherals from devicetree
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
const struct device* flash = DEVICE_DT_GET(FLASH_NODE);

/// Statistics 

/**
 * @brief LOG_PRINTK the size, dividing to KiB or MiB if large enough
 * @param size	size to print
 */
void print_size(size_t size) {
	if (size < 1024) {
		LOG_PRINTK("%ud B", size);
	} else if (size < 1024 * 1024) {
		LOG_PRINTK("%.2f KiB", ((double) size) / 1024);
	} else {
		LOG_PRINTK("%.2f MiB", ((double) size) / (1024 * 1024));
	}
}

/**
 * @brief Print the output of fs_tell
 * @param file	pointer to currently open file structure
 */
void print_tell(struct fs_file_t* file) {
	off_t cur = fs_tell(file);
	LOG_INF("Tell: %ld", cur);
}

/**
 * @brief Print the stats of a file
 * @param fname		file name
 */
void print_stat(char* fname) {
	struct fs_dirent stat_dst; // destination for directory entry (file) stats
	int32_t ret = fs_stat(fname, &stat_dst);
	if (ret < 0) {
		LOG_ERR("Unable to stat %s: %d", fname, ret);
	} else {
		LOG_INF("%s is a %s of size %d", 
				stat_dst.name,
				stat_dst.type == FS_DIR_ENTRY_FILE ? "file" : "directory",
				stat_dst.size);
	}
}


/**
 * @brief Print the stats of the file system the file is on
 * @param fname		file name
 */
void print_statvfs(char* fname) {
	struct fs_statvfs fs_stat_dst; // destination for file system stats
	int32_t ret = fs_statvfs(fname, &fs_stat_dst);
	if (ret < 0) {
		LOG_ERR("Unable to stat the filesystem of %s: %d", fname, ret);
	} else {
		LOG_INF("%s is on a volume with \r\n\t%lu blocks (%lu free) of %lu bytes each", 
				fname, fs_stat_dst.f_blocks, fs_stat_dst.f_bfree, 
				fs_stat_dst.f_frsize);
		LOG_PRINTK("\t");
		print_size(fs_stat_dst.f_blocks * fs_stat_dst.f_frsize);
		LOG_PRINTK(" (");
		print_size(fs_stat_dst.f_bfree * fs_stat_dst.f_frsize);
		LOG_PRINTK(" free)\n");
	}
}

/**
 * @brief Clear the entire flash area
 * @param fa_id		ID of the flash area, usually a result of FIXED_PARTITION_ID
 */
int32_t clear_flash_area(uint8_t fa_id) {
	int32_t ret = 0;
	const struct flash_area* area_p;
	ret = flash_area_open(fa_id, &area_p);


	if (ret < 0) {
		LOG_ERR("Failed to open flash area %d", fa_id);
		return ret;
	}

	LOG_INF("Area %u at 0x%x on %s for %u bytes\n",
			fa_id, (uint32_t) area_p->fa_off, 
			area_p->fa_dev->name,
			area_p->fa_size);

	LOG_INF("Erasing...");
	ret = flash_area_erase(area_p, 0, area_p->fa_size);
	if (ret < 0) {
		LOG_ERR("Failed to erase flash area %d", fa_id);
		return ret;
	}
	flash_area_close(area_p);
	return ret;
}


/// Samples

/**
 * @brief Treat the first four bytes of the file and increment
 * @param fname 	File name to increment
 */
void increment_int32(char* fname) {
	int32_t ret = 0;
	int32_t count = -1;

	struct fs_file_t file;

	fs_file_t_init(&file); // zeros out all the contents
						   
	ret = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
	if (ret < 0) {
		LOG_ERR("Failed to open %s: %d\n", fname, ret);
		return;
	}

	// get the counter from the file
	ret = fs_read(&file, (uint8_t*) (&count), sizeof(count));
	if (ret < 0) {
		LOG_ERR("Failed to read counter: %d\n", ret);
		goto exit;
	}

	count++;
	LOG_INF("Boot counter value: %d", count);

	// go back to the start of the file
	ret = fs_seek(&file, 0, FS_SEEK_SET);
	if (ret < 0) {
		LOG_ERR("Failed to seek to start: %d\n", ret);
		goto exit;
	}

	// write the counter to the file
	ret = fs_write(&file, (uint8_t*) (&count), sizeof(count));
	if (ret < 0) {
		LOG_ERR("Failed to write new count: %d\n", ret);
		goto exit;
	}
	
exit:
	ret = fs_close(&file);
	if (ret < 0) {
		LOG_ERR("Failed to close %s: %d\n", fname, ret);
	}
	return;
}

void write_test_pattern(char* fname) {
	int32_t ret = 0;
	char test_pattern[] = "Flash testing pattern 1";
	struct fs_file_t file;
	fs_file_t_init(&file);

	ret = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
	if (ret < 0) {
		LOG_ERR("Failed to open %s: %d\n", fname, ret);
		return;
	}
	
	// not ret-checking the rest, because if open() worked the rest probably do
	fs_truncate(&file, sizeof(test_pattern));
	fs_write(&file, (void*) test_pattern, sizeof(test_pattern));
	
	LOG_INF("File and filesystem info:");
	print_tell(&file);
	print_statvfs(fname);

	fs_truncate(&file, 2 * sizeof(test_pattern));
	LOG_INF("File and filesystem info after expansion:");
	print_tell(&file);
	print_stat(fname);
	print_statvfs(fname);
	
	fs_close(&file);
}

void read_test_pattern(char * fname) {
	int32_t ret = 0;
	char test_pattern[256];
	
	struct fs_file_t file;
	fs_file_t_init(&file);

	ret = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
	if (ret < 0) {
		LOG_ERR("Failed to open %s: %d\n", fname, ret);
		return;
	}
	
	ret = fs_read(&file, test_pattern, sizeof(test_pattern));
	if (ret < 0) {
		LOG_ERR("Failed to read from %s: %d\n", fname, ret);
	} else {
		LOG_INF("Read test pattern: %s\n", test_pattern);
	}


	fs_close(&file);
}

int main(void) {
	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("GPIO is not ready\n");
		return 0;
	}

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) {
		LOG_ERR("Unable to configure LED output pin\n");
		return 0;
	}

	if (!device_is_ready(flash)) {
		LOG_ERR("Device %s is not ready\n", flash->name);
		return 0;
	}

	char fname_1[] = "/lfs1/boot_count";
	char fname_2[] = "/lfs2/boot_count";
	char fname_3[] = "/lfs3/test_pattern";
	int32_t ret = 0;

	
	// LFS1 activities
	increment_int32(fname_1);
	print_stat(fname_1);
	print_statvfs(fname_1);
	
	// LFS2 activities
	ret = fs_mount(lfs2_mnt_p);
	if (ret < 0) {
		LOG_ERR("Failed to mount LFS 2: %d", ret);
		goto lfs3;
	}
	increment_int32(fname_2);
	
lfs3:
	ret = fs_mount(&lfs3_mnt);
	if (ret < 0) {
		LOG_ERR("Failed to mount LFS 3: %d", ret);
		goto forever;
	}
	write_test_pattern(fname_3);
	read_test_pattern(fname_3);

forever:
	// increment_int32(fname_2);

	// forever
	// just proves that the thing is alive
	while(1) {
		gpio_pin_toggle_dt(&led);
		k_msleep(100);
	}
	return 0;
}
