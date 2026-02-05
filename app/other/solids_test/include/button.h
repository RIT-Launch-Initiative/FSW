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
 * @brief Thread to beep continuously if key switch is closed and test is not started
 */
void buzzer_task(void*, void*, void*);

#endif // BUTTON_H