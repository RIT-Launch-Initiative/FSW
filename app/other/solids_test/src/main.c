#include "control.h"
#include "buzzer.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define TX_NODE DT_NODELABEL(tx)
#define RX_NODE DT_NODELABEL(rx)
static const struct gpio_dt_spec tx = GPIO_DT_SPEC_GET(TX_NODE, gpios);
static const struct gpio_dt_spec rx = GPIO_DT_SPEC_GET(RX_NODE, gpios);
static struct gpio_callback button_cb_data;

void button_pressed (const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	LOG_INF("Starting test...");
	control_start_test();
}

void button_init() {
    int ret = gpio_pin_configure_dt(&rx, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Failed to conf rx:(");
        return;
    }
    ret = gpio_pin_configure_dt(&tx, GPIO_INPUT);
    if (ret < 0) {
        printk("Failed to conf tx:(");
        return;
    }
}

int main (void) {
	LOG_INF("Solids Test Start");

	control_init();
	buzzer_init();
	button_init();
	
	LOG_INF("Use 'test start' to begin test");
    LOG_INF("Commands: test start | test stop | test dump");

	if (!gpio_is_ready_dt(&tx)) {
		LOG_INF("Error: button device %s is not ready\n", tx.port->name);
		// return 0;
	}

	int ret = gpio_pin_configure_dt(&tx, GPIO_INPUT);
	if (ret != 0) {
		LOG_INF("Error %d: failed to configure %s pin %d\n", ret, tx.port->name, tx.pin);
		// return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&tx, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_INF("Error %d: failed to configure interrupt on %s pin %d\n", ret, tx.port->name, tx.pin);
		// return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(tx.pin));
	gpio_add_callback(tx.port, &button_cb_data);

	return 0;
}