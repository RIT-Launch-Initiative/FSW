#pragma once

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/**
 * Configure UART to use RS485 flow control
 * @param dev - UART to configure
 * @return Zephyr status code
 */
int l_uart_init_rs485(const struct device *dev);



