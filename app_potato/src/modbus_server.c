// Self Include
#include "potato.h"

// Launch Includes
#include <launch_core/backplane_defs.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/modbus/modbus.h>
#include <zephyr/logging/log.h>

#define MODBUS_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_modbus_serial)

LOG_MODULE_REGISTER(modbus_server, LOG_LEVEL_INF);

static uint16_t input_reg[8] = {0};
static uint8_t event_byte;

// Note: This function does not adhere to the Modbus standard
// Simplest way to notify POTATO that boost occurred though over RS485
static int coil_wr(uint16_t addr, bool boost_detected) {
    if (addr != 0) {
        return -ENOTSUP;
    }

    event_byte = boost_detected ? L_BOOST_DETECTED : 0;

    return 0;
}

static int input_reg_rd(uint16_t addr, uint16_t* reg) {
    if (addr >= ARRAY_SIZE(input_reg)) {
        return -ENOTSUP;
    }

    *reg = input_reg[addr];

    LOG_INF("Input register read, addr %u", addr);

    return 0;
}

int insert_adc_data_to_input_reg(uint16_t addr, adc_data_t* data) {
    if (addr + 2 >= ARRAY_SIZE(input_reg)) {
        return -ENOTSUP;
    }

    input_reg[addr] = data[0];
    input_reg[addr + 1] = data[1];
    input_reg[addr + 2] = data[2];

    return 0;
}

int insert_float_to_input_reg(uint16_t addr, float value) {
    uint16_t reg[2] = {0};

    if (addr + 1 >= ARRAY_SIZE(input_reg)) {
        return -ENOTSUP;
    }

    memcpy(&reg[addr], &value, sizeof(uint16_t));

    return 0;
}


int init_modbus_server(void) {
    static struct modbus_user_callbacks mbs_cbs = {
        .input_reg_rd = input_reg_rd,
        .coil_wr = coil_wr
    };

    const static struct modbus_iface_param server_param = {
        .mode = MODBUS_MODE_RTU,
        .server = {
            .user_cb = &mbs_cbs,
            .unit_id = 1,
        },
        .serial = {
            .baud = 115200,
            .parity = UART_CFG_PARITY_NONE,
        },
    };

    const char iface_name[] = {DEVICE_DT_NAME(MODBUS_NODE)};
    int iface;

    iface = modbus_iface_get_by_name(iface_name);

    if (iface < 0) {
        LOG_ERR("Failed to get iface index for %s", iface_name);
        return iface;
    }

    return modbus_init_server(iface, server_param);
}
