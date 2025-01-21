#include <f_core/device/c_gpio.h>
#include <zephyr/drivers/gpio.h>

int CGpio::PinGet() const {
    return gpio_pin_get_dt(gpioDev);
}

int CGpio::PinSet(int value) {
    return gpio_pin_set_dt(gpioDev, value);
}

int CGpio::PinToggle() {
    return gpio_pin_toggle_dt(gpioDev);
}
