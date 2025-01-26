#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#define LILYGO_KB_BRIGHTNESS_CMD       0x01
#define LILYGO_KB_ALT_B_BRIGHTNESS_CMD 0x02

#define DT_DRV_COMPAT lilygo_tdeck_keyboard

LOG_MODULE_REGISTER(tdeck_keyboard, CONFIG_INPUT_LOG_LEVEL);

// input report key

struct tdeck_kbd_config {
    struct i2c_dt_spec bus;
    struct gpio_dt_spec interrupt_pin;
    uint8_t backlight_brightness;
    uint8_t initial_brightness;
    struct gpio_callback callback;
};

struct tdeck_kbd_data {};

// static void tdeck_keyboard_interrupt(const struct device *dev, struct gpio_callback *cbdata,
// uint32_t pins)

int isr_cb() {
    // tell thread to read
    // k workqueue submit
    return 0;
}

int myc = 0;
static void gpio_keys_interrupt(const struct device *dev, struct gpio_callback *cbdata, uint32_t pins) {
    // struct gpio_keys_callback *keys_cb = CONTAINER_OF(cbdata, struct gpio_keys_callback, gpio_cb);
    // struct gpio_keys_pin_data *pin_data = CONTAINER_OF(keys_cb, struct gpio_keys_pin_data, cb_data);
    // const struct gpio_keys_config *cfg = pin_data->dev->config;

    // ARG_UNUSED(dev); /* GPIO device pointer. */
    // ARG_UNUSED(pins);

    // k_work_reschedule(&pin_data->work, K_MSEC(cfg->debounce_interval_ms));
    myc++;
}

int tdeck_kbd_init(const struct device *dev) {
    const struct tdeck_kbd_config *cfg = dev->config;
    // setup interrupt that
    int ret = 0;
    gpio_init_callback(&cfg->callback, gpio_keys_interrupt, BIT(cfg->interrupt_pin.pin));
    if (ret < 0) {
        LOG_ERR("Could not init gpio callback");
        return ret;
    }

    ret = gpio_add_callback_dt(&cfg->interrupt_pin, &cfg->callback);
    if (ret < 0) {
        LOG_ERR("Could not set gpio callback");
        return ret;
    }

    // LOG_DBG("port=%s, pin=%d", gpio_spec->port->name, gpio_spec->pin);

    int res = gpio_pin_interrupt_configure_dt(&cfg->interrupt_pin, GPIO_INT_EDGE_BOTH);
    if (res < 0) {
        LOG_ERR("Unable to configure interrupt for gpio: %d", res);
        return res;
    }

    // while (true) {
    // LOG_INF("myc: %d", myc);
    // k_msleep(10);
    // }
    return 0;
}

#define LILYGO_KEYBOARD_INIT(index)                                                                                    \
    static const struct tdeck_kbd_config tdeck_kbd_config_##index = {                                                  \
        .bus = I2C_DT_SPEC_INST_GET(index),                                                                            \
        .interrupt_pin = GPIO_DT_SPEC_INST_GET(index, interrupt_gpios),                                                \
        .initial_brightness = DT_INST_PROP(index, initial_brightness),                                                 \
        .callback = {},                                                                                                \
    };                                                                                                                 \
    static struct tdeck_kbd_data tdeck_kbd_##index = {};                                                               \
    DEVICE_DT_INST_DEFINE(index, tdeck_kbd_init, NULL, &tdeck_kbd_##index, &tdeck_kbd_config_##index, POST_KERNEL,     \
                          CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(LILYGO_KEYBOARD_INIT)
