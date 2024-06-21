// Self Include
#include "potato.h"

// Launch Includes
#include <launch_core/os/fs.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_potato);

// Global Variables
uint32_t boot_count = -1;
bool boost_detected = false;


int main() {
    static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
    static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
    gpio_pin_set_dt(&led0, 1);
    gpio_pin_set_dt(&led1, 1);

    boot_count = l_fs_boot_count_check();
    return 0;
}
