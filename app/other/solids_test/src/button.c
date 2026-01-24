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
#define SWITCH_NODE DT_NODELABEL(key_switch)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static const struct gpio_dt_spec key_switch = GPIO_DT_SPEC_GET(SWITCH_NODE, gpios);

void buzzer_task(void*, void*, void*);
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void key_switch_state(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

static struct gpio_callback button_cb_data;
static struct gpio_callback switch_cb_data;

static bool key_switched = false;
static bool buzzing = false;

int button_switch_init() {
    int ret = gpio_pin_configure_dt(&key_switch, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Failed to conf key switch (rx) :(");
        return -1;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
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

    ret = gpio_pin_interrupt_configure_dt(&key_switch, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n", ret, key_switch.port->name, key_switch.pin);
        return -1;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

    gpio_init_callback(&switch_cb_data, key_switch_state, BIT(key_switch.pin));
	gpio_add_callback(key_switch.port, &switch_cb_data);

    return 0;
}

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	LOG_INF("Starting test...");
	control_start_test("", false);
}

void key_switch_state(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    int val = gpio_pin_get_dt(&key_switch);
    if (val > 0) {
        key_switched = true;
        set_ematch(1);
        LOG_INF("Key switch closed");

        if (!control_get_test_status()) {
            buzzing = true; // scream until test start
        }
    } else {
        key_switched = false;
        buzzing = false;
        set_buzz(0);
        set_ematch(0);
        LOG_INF("Key switch open");
    }
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