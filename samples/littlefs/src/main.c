#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

// node identifiers
// NOTE: Hardware prerequisites
// 	Launch Mikroe Click Shield
// 	W25Q128JV breakout connected to port 1

// devicetree gets
#define LED_NODE DT_ALIAS(led)
#define FLASH_NODE DT_ALIAS(storage)

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
const struct device *const flash = DEVICE_DT_GET(FLASH_NODE);

int32_t increment_file_int32(char* fname, int32_t* count) {
	int32_t ret;
	int32_t close_ret;
	struct fs_file_t file;


	// prepare file for usage
	fs_file_t_init(&file);
	ret = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
	if (ret < 0) {
		printk("Failed to open %s: %d\n", fname, ret);
		return ret;
	}

	// get the counter from the file
	ret = fs_read(&file, count, sizeof(*count));
	if (ret < 0) {
		printk("Failed to read counter: %d\n", ret);
		goto exit;
	}

	(*count)++;

	// go back to the start of the file
	ret = fs_seek(&file, 0, FS_SEEK_SET);
	if (ret < 0) {
		printk("Failed to seek to start: %d\n", ret);
		goto exit;
	}

	// write the counter to the file
	ret = fs_write(&file, count, sizeof(*count));
	if (ret < 0) {
		printk("Failed to write new count: %d\n", ret);
	}
	
exit:
	close_ret = fs_close(&file);
	if (close_ret < 0) {
		printk("Failed to close %s: %d\n", fname, close_ret);
		return close_ret;
	}
	return ret;
}

int main(void) {
	int32_t ret = 0;
	// struct sensor_value mag_field[3];

	int32_t boot_counter = -1;

	// Boilerplate: set up GPIO
	if (!gpio_is_ready_dt(&led)) {
		printk("GPIO is not ready\n");
		return 0;
	}

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) {
		printk("Unable to configure LED output pin\n");
		return 0;
	}

	if (!device_is_ready(flash)) {
		printk("Device %s is not ready\n", flash->name);
		return 0;
	}
#ifdef CONFIG_CLEAR_STORAGE_PARTITION
	flash_erase(flash, 0, DT_PROP(FLASH_NODE, size));
#endif

	ret = increment_file_int32("/lfs/boot_count", &boot_counter);
	if (ret >= 0) {
		printk("Successfully read and updated boot counter: %d boots\n", boot_counter);
	} else {
		printk("Failed to read file\n");
	}

	// forever
	// just proves that the thing is alive
	while(1) {
		gpio_pin_toggle_dt(&led);
		k_msleep(100);
	}
	return 0;
}
