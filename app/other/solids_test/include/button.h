#ifndef BUTTON_H
#define BUTTON_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <stdint.h>

/**
 * @brief Configures TX and RX pins as gpio
 * @return 0 if successful
 */
int button_init();

/**
 * @brief Interrupt function that starts a test when TX pin is pulled high
 */
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

#endif // BUTTON_H