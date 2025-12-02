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

#define TX_NODE DT_NODELABEL(tx)
#define RX_NODE DT_NODELABEL(rx)
static const struct gpio_dt_spec tx = GPIO_DT_SPEC_GET(TX_NODE, gpios);
static const struct gpio_dt_spec rx = GPIO_DT_SPEC_GET(RX_NODE, gpios);

static struct gpio_callback button_cb_data;
static struct gpio_callback switch_cb_data;

static bool key_switched = false;
static bool buzzing = false;

K_THREAD_DEFINE(buzz_thread, 512, buzzer_task, NULL, NULL, NULL, 10, 0, 0);

// tx = button, rx = key switch
int button_switch_init() {
    int ret = gpio_pin_configure_dt(&rx, GPIO_INPUT);
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

    ret = gpio_pin_interrupt_configure_dt(&rx, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n", ret, rx.port->name, rx.pin);
        return -1;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(tx.pin));
	gpio_add_callback(tx.port, &button_cb_data);

    gpio_init_callback(&switch_cb_data, key_switch_state, BIT(rx.pin));
	gpio_add_callback(rx.port, &switch_cb_data);

    return 0;
}

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	LOG_INF("Starting test...");
	control_start_test("", false);
}

void key_switch_state(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    int val = gpio_pin_get_dt(&rx);
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

void buzzer_task() {
    while (1) {
        if (buzzing && !control_get_test_status()) {
            continuous_beep();
        } else {
            set_buzz(0);
            k_msleep(50);
        }
    }
}