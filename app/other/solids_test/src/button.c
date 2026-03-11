#include "button.h"
#include "control.h"
#include "buzzer.h"

#include <stdbool.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <stdint.h>

LOG_MODULE_REGISTER(button, LOG_LEVEL_INF);

// button = tx, key switch = rx
#define BUTTON_NODE DT_NODELABEL(button)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);

void buzzer_task(void*, void*, void*);
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

static struct gpio_callback button_cb_data;

static bool buzzing = false;

int button_switch_init() {
    int ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Failed to conf button (tx) :(");
        return -1;
    }

    if (!gpio_is_ready_dt(&button)) {
		LOG_ERR("Error: button device %s is not ready\n", button.port->name);
        return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin);
        return -1;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

    return 0;
}

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	LOG_INF("Starting test...");
	control_start_test("", false);
}


void buzzer_task(void*, void*, void*) {
    while (true) {
        if (buzzing && !control_get_test_status()) {
            continuous_beep();
        } else {
            set_buzz(0);
            k_msleep(50);
        }
    }
}