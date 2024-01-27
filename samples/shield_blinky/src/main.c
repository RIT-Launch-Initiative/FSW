#include "zephyr/logging/log.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/gpio.h>
#include <stdio.h>


// devicetree gets
#define LED_NODE DT_ALIAS(led)

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

int main(void) {
	int counter = 0;
	// Boilerplate: set up GPIO
	if (!gpio_is_ready_dt(&led)) {
		printf("GPIO is not ready\n");
		return 0;
	}

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) {
		printf("Unable to configure LED output pin\n");
		return 0;
	}

	// forever
	while(1) {
		gpio_pin_toggle_dt(&led);
		LOG_PRINTK("LED toggle %d\n", ++counter);
		k_msleep(100);
	}
	return 0;
}
