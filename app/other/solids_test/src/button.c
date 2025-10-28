#include "button.h"
#include "control.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <stdint.h>

LOG_MODULE_REGISTER(button, LOG_LEVEL_INF);

#define TX_NODE DT_NODELABEL(tx)
#define RX_NODE DT_NODELABEL(rx)
static const struct gpio_dt_spec tx = GPIO_DT_SPEC_GET(TX_NODE, gpios);
static const struct gpio_dt_spec rx = GPIO_DT_SPEC_GET(RX_NODE, gpios);
static struct gpio_callback button_cb_data;

int button_init(void) {
    int ret = gpio_pin_configure_dt(&rx, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to conf rx :(");
        return -1;
    }

    ret = gpio_pin_configure_dt(&tx, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Failed to conf tx :(");
        return -1;
    }

    if (!gpio_is_ready_dt(&tx)) {
		LOG_ERR("Error: button device %s is not ready\n", tx.port->name);
        return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&tx, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n", ret, tx.port->name, tx.pin);
        return -1;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(tx.pin));
	gpio_add_callback(tx.port, &button_cb_data);

    return 0;
}

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	LOG_INF("Starting test...");
	control_start_test();
}