#include "control.h"
#include "buzzer.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(tx), gpios);
static struct gpio_callback button_cb_data;

void button_pressed (const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	shell_print(shell, "Starting test...");
	control_start_test();
}

int main (void) {
	LOG_INF("Solids Test Start");

	control_init();
	buzzer_init();
	LOG_INF("Use 'test start' to begin test");
    LOG_INF("Commands: test start | test stop | test dump");

	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n", button.port->name);
		return 0;
	}

	int ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin);
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	return 0;
}