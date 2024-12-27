#include <f_core/device/c_gpio.h>
#include <zephyr/drivers/gpio.h>

CGpio::CGpio(const device& dev) : dev(&dev) {}

CGpio::pin_get() {
    return gpio_pin_get_dt(&dev);
}

CGpio::pin_set(int value) {
    return gpio_pin_set_dt(&dev, value)
}