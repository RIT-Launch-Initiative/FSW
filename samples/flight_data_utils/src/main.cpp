#include "zephyr/fs/fs_interface.h"
#include <cstdint>
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

struct txyz {
	int32_t time;
	struct sensor_value xyz[3];
};
struct txyz mag_val;
char once_path[] = "/lfs/once";
char circ_path[] = "/lfs/circ";
char bc_path[] = "/lfs/boot_count";
#define N_SAMPLES 100

SensorLogger once_logger{once_path, sizeof(mag_val), sizeof(mag_val) * N_SAMPLES, SLOG_ONCE};
SensorLogger circ_logger{circ_path, sizeof(mag_val), sizeof(mag_val) * N_SAMPLES, SLOG_CIRC};

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

void print_mag_val() {
	LOG_PRINTK("T: %d ct\tX: %5f ga\tY: %5f ga\tZ: %5f ga\n",
			mag_val.time, 
			sensor_value_to_double(mag_val.xyz),
			sensor_value_to_double(mag_val.xyz + 1),
			sensor_value_to_double(mag_val.xyz + 2));
}

int32_t read_mag(struct txyz* val) {
	int32_t ret = 0;

	ret = sensor_sample_fetch(mag);
	if (ret < 0) {
		LOG_ERR("Unable to fetch sensor sample: %d\n", ret);
		return ret;
	}

	val->time = k_uptime_ticks();

	ret = sensor_channel_get(mag, SENSOR_CHAN_MAGN_XYZ, val->xyz);
	if (ret < 0) {
		LOG_ERR("Unable to read sensor channel: %d\n", ret);
		return ret;
	}
	
	return 0;
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

	ret = once_logger.init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize oneshot logger: %d", ret);
		return ret;
	}

	ret = circ_logger.init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize circular logger: %d", ret);
		return ret;
	}

	ret = increment_file_int32(bc_path, &boot_counter);
	if (ret >= 0) {
		LOG_INF("Successfully read and updated boot counter: %d boots", boot_counter);
	} else {
		LOG_ERR("Failed to read file");
	}

	int32_t once_status = 0;
	int32_t circ_status = 0;
	uint32_t now = k_uptime_ticks();
	while (k_uptime_ticks() < (now + CONFIG_SYS_CLOCK_TICKS_PER_SEC * 10)) {
		ret = read_mag(&mag_val);
		if (ret < 0) {
			continue;
		}
		if (once_status >= 0) {
			once_status = once_logger.write((uint8_t*) &mag_val);
		}
		if (circ_status >= 0) {
			circ_status = circ_logger.write((uint8_t*) &mag_val);
		}
	}

	LOG_INF("Printing once-logged values:");
	for (size_t i = 0; i < N_SAMPLES; i++) {
		once_logger.read((uint8_t*) &mag_val, i);
		print_mag_val();
	}
	LOG_INF("Printing circular-logged values:");
	for (size_t i = 0; i < N_SAMPLES; i++) {
		circ_logger.read((uint8_t*) &mag_val, i);
		print_mag_val();
	}

	// forever
	// just proves that the thing is alive
	while(1) {
		gpio_pin_toggle_dt(&led);
		k_msleep(100);
	}
	return 0;
}
