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
 * @param[in] dev GPIO device
 * @param[in] cb Callback structure pointer
 * @param[in] pins Interrupt pins
 */
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

/**
 * @brief Key switch interrupt function that sets ematch and checks test status
 *        If key switch is open and test is not started, buzzer will beep continuously
 * @param[in] dev GPIO device
 * @param[in] cb Callback structure pointer
 * @param[in] pins Interrupt pins
 */
void key_switch_state(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

/**
 * @brief Thread to beep continuously if key switch is closed and test is not started
 */
void buzzer_task();

#endif // BUTTON_H