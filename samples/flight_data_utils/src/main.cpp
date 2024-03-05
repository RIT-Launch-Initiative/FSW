#include "zephyr/fs/fs_interface.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#include <launch_core/os/Logger.h>
// node identifiers
// NOTE: Hardware prerequisites
// 	Launch Mikroe Click Shield
// 	W25Q128JV breakout connected to port 1

LOG_MODULE_REGISTER(sample, CONFIG_LOG_DEFAULT_LEVEL);

// devicetree gets
#define LED_NODE DT_ALIAS(led)
#define FLASH_NODE DT_ALIAS(storage)
#define MAG_NODE DT_ALIAS(mag)

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
const struct device* const flash = DEVICE_DT_GET(FLASH_NODE);
const struct device* const mag = DEVICE_DT_GET(MAG_NODE);

struct sensor_value mag_val[3];

char mag_path[] = "/lfs/mag";
char bootcount_path[] = "/lfs/boot_count";
SensorLogger inf_logger{mag_path, sizeof(mag_val), SLOG_INFINITE};

int32_t increment_file_int32(char* fname, int32_t* count) {
	int32_t ret;
	int32_t close_ret;
	struct fs_file_t file;


	// prepare file for usage
	fs_file_t_init(&file);
	ret = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
	if (ret < 0) {
		LOG_ERR("Failed to open %s: %d\n", fname, ret);
		return ret;
	}

	// get the counter from the file
	ret = fs_read(&file, count, sizeof(*count));
	if (ret < 0) {
		LOG_ERR("Failed to read counter: %d\n", ret);
		goto exit;
	}

	(*count)++;

	// go back to the start of the file
	ret = fs_seek(&file, 0, FS_SEEK_SET);
	if (ret < 0) {
		LOG_ERR("Failed to seek to start: %d\n", ret);
		goto exit;
	}

	// write the counter to the file
	ret = fs_write(&file, count, sizeof(*count));
	if (ret < 0) {
		LOG_ERR("Failed to write new count: %d\n", ret);
	}
	
exit:
	close_ret = fs_close(&file);
	if (close_ret < 0) {
		LOG_ERR("Failed to close %s: %d\n", fname, close_ret);
		return close_ret;
	}
	return ret;
}

void print_mag(struct sensor_value* val) {
	LOG_PRINTK("X: %f\tY: %f\tZ: %f\n", 
			sensor_value_to_double(val), 
			sensor_value_to_double(val + 1), 
			sensor_value_to_double(val + 2));
}

int32_t read_mag(struct sensor_value* val) {
	int32_t ret = 0;

	ret = sensor_sample_fetch(mag);
	if (ret < 0) {
		LOG_ERR("Unable to fetch sensor sample: %d\n", ret);
		return ret;
	}

	ret = sensor_channel_get(mag, SENSOR_CHAN_MAGN_XYZ, val);
	if (ret < 0) {
		LOG_ERR("Unable to read sensor channel: %d\n", ret);
		return ret;
	}

	print_mag(val);
	
	return 0;
}

void log_mag(struct sensor_value* val) {
	int32_t ret = 0;

	ret = inf_logger.write(reinterpret_cast<uint8_t *>(val));

	if (ret < 0) {
		LOG_ERR("Failed to write flight data: %d", ret);
	}
}

void mag_task(void) {
	int n_loops = 0;
	while(n_loops++ < 1000*60) {
		read_mag(mag_val);
		k_msleep(1);
	}
}

void log_task(void) {
	int n_loops = 0;
	while(n_loops++ < 1000*60) {
		log_mag(mag_val);
		k_msleep(1);
	}
}

int main(void) {
	int32_t ret = 0;
	// struct sensor_value mag_field[3];

	int32_t boot_counter = -1;

	// Boilerplate: set up GPIO
	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("GPIO is not ready");
		return -1;
	}

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) {
		LOG_ERR("Unable to configure LED output pin");
		return -1;
	}

	if (!device_is_ready(flash)) {
		LOG_ERR("Device %s is not ready", flash->name);
		return -1;
	}

	if (!device_is_ready(mag)) {
		LOG_ERR("Device %s is not ready", mag->name);
		return -1;
	}

	ret = inf_logger.init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize logger: %d\n", ret);
		return ret;
	}

	ret = increment_file_int32(bootcount_path, &boot_counter);
	if (ret >= 0) {
		LOG_ERR("Successfully read and updated boot counter: %d boots\n", boot_counter);
	} else {
		LOG_ERR("Failed to read file\n");
	}

	// forever
	// just proves that the thing is alive
	while(1) {
		gpio_pin_toggle_dt(&led);
		k_msleep(100);
	}
	return 0;
}

#define STACKSIZE 1024
K_THREAD_DEFINE(mag_task_id, STACKSIZE, mag_task, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(log_task_id, STACKSIZE, log_task, NULL, NULL, NULL, 7, 0, 0);
