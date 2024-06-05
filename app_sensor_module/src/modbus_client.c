// Self Include
#include "sensor_module.h"

// Launch Include
#include <launch_core/backplane_defs.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/modbus/modbus.h>
#include <zephyr/logging/log.h>

// TODO: Use backplane_defs in other PR once merged
#define POTATO_NODE 1
#define BOOST_DETECT_ADDR 0
#define LPS22_PRESSURE_REGISTER 1
#define LPS22_TEMPERATURE_REGISTER LPS22_PRESSURE_REGISTER + 1
#define ADC_REGISTER LPS22_TEMPERATURE_REGISTER + 1

LOG_MODULE_REGISTER(modbus_client);

// Globals
static bool modbus_initialized = false;
int modbus_client_iface;

static const struct modbus_iface_param client_param = {
    .mode = MODBUS_MODE_RTU,
    .rx_timeout = 50000,
    .serial = {
        .baud = 115200,
        .parity = UART_CFG_PARITY_NONE,
        .stop_bits_client = UART_CFG_STOP_BITS_2,
    },
};

int init_modbus_client(void) {
    const char iface_name[] = {DEVICE_DT_NAME(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_modbus_serial))};

    modbus_client_iface = modbus_iface_get_by_name(iface_name);

    int ret = modbus_init_client(modbus_client_iface, client_param);
    modbus_initialized = ret == 0;
    return ret;
}

int write_boost_detect_byte_modbus(uint8_t event_byte) {
    return modbus_write_coil(modbus_client_iface, POTATO_NODE, BOOST_DETECT_ADDR, event_byte == L_BOOST_DETECTED);
}

int read_potato_telemetry(float *pressure, float *temperature, float *load) {
    float data[3] = {0};
    return modbus_read_input_regs(modbus_client_iface, POTATO_NODE, LPS22_PRESSURE_REGISTER, (uint16_t *) data, sizeof(data));
}