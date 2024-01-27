#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
// #include <zephyr/fs/littlefs.h>

// #include <string.h>
#include <stdio.h>

// node identifiers
// NOTE: Hardware prerequisites
// 	Launch Mikroe Click Shield
// 	LIS3MDL connected to shield i2c
// 	W25Q128JV breakout connected to port 1

// devicetree gets
#define LED_NODE DT_ALIAS(led)
#define MAG_NODE DT_ALIAS(magn)
#define FLASH_NODE DT_ALIAS(storage)

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
const struct device *const mag = DEVICE_DT_GET(MAG_NODE);
const struct device *const flash = DEVICE_DT_GET(FLASH_NODE);

int32_t increment_file_int32(char* fname, int32_t* count) {
	int32_t ret;
	int32_t close_ret;
	struct fs_file_t file;


	// prepare file for usage
	fs_file_t_init(&file);
	ret = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
	if (ret < 0) {
		LOG_PRINTK("Failed to open %s: %d\n", fname, ret);
		return ret;
	}

	// get the counter from the file
	ret = fs_read(&file, count, sizeof(*count));
	if (ret < 0) {
		LOG_PRINTK("Failed to read counter: %d\n", ret);
		goto exit;
	}

	(*count)++;

	// go back to the start of the file
	ret = fs_seek(&file, 0, FS_SEEK_SET);
	if (ret < 0) {
		LOG_PRINTK("Failed to seek to start: %d\n", ret);
		goto exit;
	}

	// write the counter to the file
	ret = fs_write(&file, count, sizeof(*count));
	if (ret < 0) {
		LOG_PRINTK("Failed to write new count: %d\n", ret);
	}
	
exit:
	close_ret = fs_close(&file);
	if (close_ret < 0) {
		LOG_PRINTK("Failed to close %s: %d\n", fname, close_ret);
		return close_ret;
	}
	return ret;
}

int32_t read_mag(struct sensor_value* val) {
	int32_t ret = 0;

	ret = sensor_sample_fetch(mag);
	if (ret < 0) {
		LOG_PRINTK("Unable to fetch sensor sample: %d\n", ret);
		return ret;
	}

	ret = sensor_channel_get(mag, SENSOR_CHAN_MAGN_XYZ, val);
	if (ret < 0) {
		LOG_PRINTK("Unable to read sensor channel: %d\n", ret);
		return ret;
	}
	
	LOG_PRINTK("X: %f\tY: %f\tZ: %f\n", 
			sensor_value_to_double(val), 
			sensor_value_to_double(val + 1), 
			sensor_value_to_double(val + 2));

	return 0;
}

int main(void) {
	int32_t ret = 0;
	// struct sensor_value mag_field[3];

	int32_t boot_counter = -1;

	// Boilerplate: set up GPIO
	if (!gpio_is_ready_dt(&led)) {
		printf("GPIO is not ready\n");
		return 0;
	}

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) {
		printf("Unable to configure LED output pin\n");
		return 0;
	}

	if (!device_is_ready(mag)) {
		printf("Device %s is not ready\n", mag->name);
		return 0;
	}

#ifdef CONFIG_CLEAR_STORAGE_PARTITION
	if (!device_is_ready(flash)) {
		printf("Device %s is not ready\n", flash->name);
		return 0;
	}
	flash_erase(flash, 0, DT_PROP(FLASH_NODE, size));
#endif

	ret = increment_file_int32("/lfs/boot_count", &boot_counter);
	if (ret >= 0) {
		LOG_PRINTK("Successfully read and updated boot counter: %d boots\n", boot_counter);
	} else {
		LOG_PRINTK("Failed to read file\n");
	}

	// forever
	while(1) {
		gpio_pin_toggle_dt(&led);
		// read_mag(mag_field);
		k_msleep(100);
	}
	return 0;
}
