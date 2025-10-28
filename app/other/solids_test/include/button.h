#ifndef BUTTON_H
#define BUTTON_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <stdint.h>

int button_init(void);
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

#endif // BUTTON_H