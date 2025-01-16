#include <f_core/device/c_gpio.h>
#include <zephyr/drivers/gpio.h>

int CGpio::pin_get() const {
    return gpio_pin_get_dt(gpioDev);
}

int CGpio::pin_set(int value) {
    return gpio_pin_set_dt(gpioDev, value);
}

int CGpio::pin_toggle() {
    return gpio_pin_toggle_dt(gpioDev);
}
