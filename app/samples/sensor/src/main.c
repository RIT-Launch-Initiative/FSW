#include "zephyr/kernel/thread_stack.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sample, CONFIG_LOG_DEFAULT_LEVEL);

// devicetree gets
#define LED_NODE DT_ALIAS(led)
#define MAG_NODE DT_ALIAS(magn)

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
const struct device *const mag = DEVICE_DT_GET(MAG_NODE);

K_THREAD_STACK_DEFINE(read_mag_stack, 1024);
struct k_thread read_mag_thread_data;
// NOTE: we can't use K_THREAD_DEFINE because we want the thread to only start
// if everything is successful, not just based on a delay

int32_t read_mag() {
	int32_t ret = 0;
	struct sensor_value val[3];

	ret = sensor_sample_fetch(mag);
	if (ret < 0) {
		LOG_INF("Unable to fetch sensor sample: %d\n", ret);
		return ret;
	}

	ret = sensor_channel_get(mag, SENSOR_CHAN_MAGN_XYZ, val);
	if (ret < 0) {
		LOG_INF("Unable to read sensor channel: %d\n", ret);
		return ret;
	}
	
	LOG_PRINTK("KM: %6d ms\tKT: %10d tck\tCK: %10u cyc\tX: %4fga\tY: %4fga\tZ: %4fga\n", 
			k_uptime_get_32(),
			(int32_t) k_uptime_ticks(),
			k_cycle_get_32(),
			sensor_value_to_double(val), 
			sensor_value_to_double(val + 1), 
			sensor_value_to_double(val + 2));

	return 0;
}

void read_mag_thread() {
	int32_t ret = 0;
	while (1) {
		ret = read_mag();
		if (ret < 0) {
			return;
		}
		k_msleep(100);
	}
}


int main(void) {
	// Boilerplate: set up GPIO
	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("GPIO is not ready\n");
		return 0;
	}

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) {
		LOG_ERR("Unable to configure LED output pin\n");
		return 0;
	}

	if (!device_is_ready(mag)) {
		LOG_ERR("Device %s is not ready\n", mag->name);
		return 0;
	}

	LOG_INF("Current ticks_per_ms is: %d", CONFIG_SYS_CLOCK_TICKS_PER_SEC);

	k_thread_create(
			&read_mag_thread_data, 
			read_mag_stack, K_THREAD_STACK_SIZEOF(read_mag_stack), 
			&read_mag_thread, NULL, NULL, NULL, 
			7, 0, K_NO_WAIT);

	// forever
	while(1) {
		gpio_pin_toggle_dt(&led);
		k_msleep(100);
	}
	return 0;
}
