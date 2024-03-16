#ifndef L_MODBUS_H_
#define L_MODBUS_H_

#include <stdint.h>

#define L_DEFAULT_MODBUS_RX_TIMEOUT 1000
#define L_DEFAULT_MODBUS_BAUD_RATE 115200
#define L_DEFAULT_MODBUS_PARITY UART_CFG_PARITY_NONE
#define L_DEFAULT_MODBUS_STOP_BITS_CLIENT UART_CFG_STOP_BITS_2

/**
 * Initialize a Modbus Client or Server
 * @param unit_id - Unit ID of the Modbus device. Set to 0 for a client.
 * @return Zephyr status code of the initialization
 */
int l_modbus_init(const uint8_t unit_id);

#endif //L_MODBUS_H_
