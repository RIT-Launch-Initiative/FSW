#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#include <launch_core/os/fs.h>
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

char bc_path[] = "/lfs/boot_count";

char boost_samples_path[] = "/lfs/boost";
char flight_samples_path[] = "/lfs/flight";
char too_big_path[] = "/lfs/throwaway";
#define N_CIRC_SAMPLES 100
#define N_ONCE_SAMPLES 300

SensorLogger detect_buffer{
	boost_samples_path, 
	sizeof(struct txyz), 
	N_CIRC_SAMPLES, 
	SLOG_CIRC);

L_FS_CREATE_FILE flight_log{
	flight_samples_path,
	sizeof(struct txyz),
	N_ONCE_SAMPLES,
	SLOG_ONCE};

// "too-big" 
/* SensorLogger too_big_logger{
	too_big_path,
	sizeof(struct txyz),
	1024 * 1024,
	SLOG_ONCE
}; */

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

void print_mag_val(struct txyz* val) {
	LOG_PRINTK("T: %d ct\tX: %5f ga\tY: %5f ga\tZ: %5f ga\n",
			val->time, 
			sensor_value_to_double(val->xyz),
			sensor_value_to_double(val->xyz + 1),
			sensor_value_to_double(val->xyz + 2));
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

int32_t log_routine(void) {
	int32_t ret = 0;
	int32_t now = k_uptime_ticks();
	int32_t end = now + 
		CONFIG_SENSOR_LOG_DURATION * CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	struct txyz sample;

	ret = detect_buffer.init();
	if (ret < 0) {
		LOG_ERR("Unable to initialize detection circular buffer: %d", ret);
		return ret;
	}
	ret = flight_log.init();
	if (ret < 0) {
		LOG_ERR("Unable to initialize flight buffer: %d", ret);
		return ret;
	}

	// Look for a large magnetic field for CONFIG... seconds
	while (k_uptime_ticks() < end) { 
		ret = read_mag(&sample);
		if (ret < 0) {
			LOG_ERR("Failed to read from magnetometer");
			return ret;
		}

		ret = detect_buffer.write((uint8_t*) &sample);
		if (ret < 0) {
			LOG_ERR("Failed to write sample");
			return ret;
		}

		if (abs(sample.xyz[2].val1) > 3) { // mag_z is large
			break;
		}
		k_msleep(10);
	}
	if (ret < 0) {
		return ret;
	}

	// flight_log errors when out of space
	while (1) {
		ret = read_mag(&sample);
		if (ret < 0) {
			LOG_ERR("Failed to read from magnetometer: %d", ret);
			return ret;
		}

		ret = flight_log.write((uint8_t*) &sample);
		if (-ENOSPC == ret) {
			break;
		} else if (ret < 0) {
			return ret;
		}
		k_msleep(10);
	}
	
	LOG_INF("Printing detection buffer");
	// print logged values
	for (off_t i = 0; i < N_CIRC_SAMPLES; i++) {
		detect_buffer.read((uint8_t*) &sample, i);
		print_mag_val(&sample);
	}


	LOG_INF("Printing flight buffer");
	for (off_t i = 0; i < N_ONCE_SAMPLES; i++) {
		flight_log.read((uint8_t*) &sample, i);
		print_mag_val(&sample);
	}

	LOG_INF("Verify that the flight buffer's timer counts pick up where the detection buffer ends.");
	return ret;
}

int main(void) {
	int32_t ret = 0;

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

	ret = increment_file_int32(bc_path, &boot_counter);
	if (ret >= 0) {
		LOG_INF("Successfully read and updated boot counter: %d boots", boot_counter);
	} else {
		LOG_ERR("Failed to read file");
	}

	/* ret = too_big_logger.init();
	if (ret == -ENOSPC) {
		LOG_INF("Logger did not have enough space to initialize");
		LOG_INF("Bad file is size %d", too_big_logger.file_size());
	} else if (ret < 0) {
		LOG_ERR("Other error: %d", ret);
	} else {
		LOG_ERR("Error expected but none recieved");
	} */

	log_routine();
	// just proves that the thing is alive
	while(1) {
		gpio_pin_toggle_dt(&led);
		k_msleep(100);
	}
	return 0;
}
