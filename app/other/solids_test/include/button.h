#ifndef BUTTON_H
#define BUTTON_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <stdint.h>

/**
 * @brief Configures TX and RX pins as gpio
 * @return 0 if successful
 */
int button_switch_init();

/**
 * @brief Interrupt function that starts a test when TX pin is pulled high
 * @param dev GPIO device
 * @param cb Callback structure pointer
 * @param pins Interrupt pins
 */
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

/**
 * @brief Interrupt function that toggles ematch and does something else
 * @param dev GPIO device
 * @param cb Callback structure pointer
 * @param pins Interrupt pins
 */
void key_switch_state(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

/**
 * @brief Thread to beep continuously if key switch is closed and test is not started
 */
void buzzer_task();

#endif // BUTTON_H