#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT awinic_aw9364

struct aw9364_config {
    struct gpio_dt_spec one_wire_gpio;
    uint8_t initial_brightness;
};
struct aw9364_data {
    uint8_t current_brightness;
};

// LilyGo  T-Deck  control backlight chip has 16 levels of adjustment range
// The adjustable range is 0~15, 0 is the minimum brightness, 15 is the maximum
// brightness
int aw9364_set_brightness(const struct device *dev, uint8_t brightness) {
    const struct aw9364_config *cfg = dev->config;
    struct aw9364_data *data = dev->data;
    if (brightness > 15) {
        return -ERANGE;
    }

    static const uint8_t steps = 16;
    if (brightness == 0) {
        gpio_pin_set_dt(&cfg->one_wire_gpio, 0);
        k_msleep(3);
        data->current_brightness = 0;
        return 0;
    }
    if (data->current_brightness == 0) {
        gpio_pin_set_dt(&cfg->one_wire_gpio, 1);
        data->current_brightness = steps;
        k_usleep(30);
    }
    int from = steps - data->current_brightness;
    int to = steps - brightness;
    int num = (steps + to - from) % steps;
    for (int i = 0; i < num; i++) {
        gpio_pin_set_dt(&cfg->one_wire_gpio, 0);
        k_usleep(1);
        gpio_pin_set_dt(&cfg->one_wire_gpio, 1);
    }
    data->current_brightness = brightness;

    return 0;
}

int aw9364_init(const struct device *dev) {
    const struct aw9364_config *cfg = dev->config;

    if (gpio_is_ready_dt(&cfg->one_wire_gpio)) {
        int err;
        err = gpio_pin_configure_dt(&cfg->one_wire_gpio, GPIO_OUTPUT);
        if (err) {
            printk("failed to configure backlight gpio: %d", err);
            return -ENODEV;
        }
    }
    return aw9364_set_brightness(dev, cfg->initial_brightness);
}

#define AW9364_INIT(index)                                                                                             \
    static const struct aw9364_config aw9364_config_##index = {                                                        \
        .initial_brightness = DT_INST_PROP(index, initial_brightness),                                                 \
        .one_wire_gpio = GPIO_DT_SPEC_INST_GET(index, one_wire_gpios),                                                 \
    };                                                                                                                 \
    static struct aw9364_data aw9364_data_##index = {0};                                                               \
    DEVICE_DT_INST_DEFINE(index, aw9364_init, NULL, &aw9364_data_##index, &aw9364_config_##index, POST_KERNEL,         \
                          CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(AW9364_INIT)
