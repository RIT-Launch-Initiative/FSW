/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/os/n_rtos.h>
#include <f_core/device/c_gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);


int main() {
    gpio_dt_spec ledPin = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
    CGpio led(ledPin);

    while (true) {
        led.pin_set(1);
        k_msleep(1000);
        led.pin_set(0);
        k_msleep(1000);
    }

    return 0;
}
