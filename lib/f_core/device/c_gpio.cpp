#include <f_core/device/c_gpio.h>
#include <zephyr/drivers/gpio.h>

int CGpio::GetPin() const {
    return gpio_pin_get_dt(gpioDev);
}

int CGpio::SetPin(int value) {
    return gpio_pin_set_dt(gpioDev, value);
}

int CGpio::TogglePin() {
    return gpio_pin_toggle_dt(gpioDev);
}

const gpio_dt_spec* CGpio::GetDev() {
    return gpioDev;
}
