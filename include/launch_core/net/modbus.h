#ifndef L_MODBUS_H_
#define L_MODBUS_H_

#include <stdint.h>

#define L_DEFAULT_MODBUS_RX_TIMEOUT 1000
#define L_DEFAULT_MODBUS_BAUD_RATE 115200
#define L_DEFAULT_MODBUS_PARITY UART_CFG_PARITY_NONE
#define L_DEFAULT_MODBUS_STOP_BITS_CLIENT UART_CFG_STOP_BITS_2

typedef struct {
    uint8_t unit_id;
    uint16_t start_addr;
    uint16_t num_regs;
} l_modbus_access_args_t;

/**
 * Initialize a Modbus Client or Server
 * @param unit_id - Unit ID of the Modbus device. Set to 0 for a client.
 * @return Zephyr status code of the initialization
 */
int l_modbus_init(const uint8_t unit_id);


int l_modbus_one_shot_read_input_registers(const l_modbus_access_args_t &args, void *data, size_t data_len);

#endif //L_MODBUS_H_
