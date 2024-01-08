#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/flash.h>

#include <stdio.h>

// node identifiers
// NOTE: Hardware prerequisites
// 	Launch Mikroe Click Shield
// 	LIS3MDL connected to shield i2c
// 	W25Q128JV breakout connected to port 1

#define LED_NODE DT_ALIAS(led)
#define MAG_NODE DT_ALIAS(magn)
#define FLASH_NODE DT_ALIAS(storage)

#define SPI_FLASH_SECTOR_SIZE 4096
#define TEST_STRING_LENGTH 256

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
const struct device *const mag = DEVICE_DT_GET(MAG_NODE);
const struct device *const flash = DEVICE_DT_GET(FLASH_NODE);


int32_t read_write_compare(off_t offset, const char *const text, size_t len) {
	int32_t ret = 0;
	uint8_t buf[TEST_STRING_LENGTH];

	printf("Testing write of %d bytes to 0x%6X\n", 
			len, (unsigned int) offset);
	printf("Erasing target sector... ");

	gpio_pin_set_dt(&led, 1);
	ret = flash_erase(flash, offset, SPI_FLASH_SECTOR_SIZE);
	gpio_pin_set_dt(&led, 0);

	if (0 == ret) {
		printf("Succeeded\n");
	} else {
		printf("Failed: %d\n", ret);
		return ret;
	}
	
	printf("Testing write... ");

	gpio_pin_set_dt(&led, 1);
	ret = flash_write(flash, offset, (uint8_t*) text, len);
	gpio_pin_set_dt(&led, 0);

	if (0 == ret) {
		printf("Succeeded.\n\tSENT: %*s\n", len, text);
	} else {
		printf("Failed: %d\n", ret);
		return ret;
	}

	memset(buf, 0, len);
	printf("Testing read... ");

	gpio_pin_set_dt(&led, 1);
	ret = flash_read(flash, offset, buf, len);
	gpio_pin_set_dt(&led, 0);

	if (0 == ret) {
		printf("Succeeded.\n\tREAD: %*s\n", len, (char*) buf);
	} else {
		printf("Failed: %d\n", ret);
		return ret;
	}

	printf("RESULT: ");
	if (0 == memcmp(text, buf, len)) {
		printf("Match.\n");
	} else {
		printf("Mismatch.\n");
	}
	
	return ret;
}

int32_t read_mag(struct sensor_value* val) {
	int32_t ret = 0;

	ret = sensor_sample_fetch(mag);
	if (ret < 0) {
		printf("Unable to fetch sensor sample: %d\n", ret);
		return ret;
	}

	ret = sensor_channel_get(mag, SENSOR_CHAN_MAGN_XYZ, val);
	if (ret < 0) {
		printf("Unable to read sensor channel: %d\n", ret);
		return ret;
	}
	
	printf("X: %f\tY: %f\tZ: %f\n", 
			sensor_value_to_double(val), 
			sensor_value_to_double(val + 1), 
			sensor_value_to_double(val + 2));

	return 0;
}

int main(void) {
	int32_t ret = 0;
	struct sensor_value mag_field[3];

	printf("Testing flash write (2).\n");

	// Boilerplate: set up GPIO
	if (!gpio_is_ready_dt(&led)) {
		printf("GPIO is not ready\n");
		return 0;
	}

	gpio_pin_set_dt(&led, 1);

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) {
		printf("Unable to configure LED output pin\n");
		return 0;
	}

	/*
	if (!device_is_ready(mag)) {
		printf("Device %s is not ready\n", mag->name);
		return 0;
	}
	*/

	if (!device_is_ready(flash)) {
		printf("Device %s is not ready\n", flash->name);
		return 0;
	}

	char text[] = "First test text";
	read_write_compare(0, text, strlen(text));
	read_write_compare(0x1000, text, strlen(text));

	char text2[] = "Second test text";
	read_write_compare(0, text2, strlen(text2));
	read_write_compare(0x1000, text2, strlen(text2));

	// forever
	while(1) {
		gpio_pin_toggle_dt(&led);
		// read_mag(mag_field);
		k_msleep(250);
	}
	return 0;
}
