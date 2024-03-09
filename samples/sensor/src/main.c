#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

// devicetree gets
#define LED_NODE DT_ALIAS(led)
#define MAG_NODE DT_ALIAS(magn)

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
const struct device *const mag = DEVICE_DT_GET(MAG_NODE);

int32_t read_mag(struct sensor_value* val) {
	int32_t ret = 0;

	ret = sensor_sample_fetch(mag);
	if (ret < 0) {
		printk("Unable to fetch sensor sample: %d\n", ret);
		return ret;
	}

	ret = sensor_channel_get(mag, SENSOR_CHAN_MAGN_XYZ, val);
	if (ret < 0) {
		printk("Unable to read sensor channel: %d\n", ret);
		return ret;
	}
	
	printk("X: %f\tY: %f\tZ: %f\n", 
			sensor_value_to_double(val), 
			sensor_value_to_double(val + 1), 
			sensor_value_to_double(val + 2));

	return 0;
}

int main(void) {
	struct sensor_value mag_field[3];

	// Boilerplate: set up GPIO
	if (!gpio_is_ready_dt(&led)) {
		printk("GPIO is not ready\n");
		return 0;
	}

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) {
		printk("Unable to configure LED output pin\n");
		return 0;
	}

	if (!device_is_ready(mag)) {
		printk("Device %s is not ready\n", mag->name);
		return 0;
	}

	// forever
	while(1) {
		gpio_pin_toggle_dt(&led);
		read_mag(mag_field);
		k_msleep(100);
	}
	return 0;
}
