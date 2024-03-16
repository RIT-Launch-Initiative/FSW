#include <launch_core/net/modbus.h>

#include <zephyr/logging/log.h>
#include <zephyr/modbus/modbus.h>

LOG_MODULE_REGISTER(launch_modbus);

int l_modbus_server_init(const uint8_t unit_id) {
    struct modbus_iface_param iface_param = {
            .mode = MODBUS_MODE_RTU,
            .rx_timeout = L_DEFAULT_MODBUS_RX_TIMEOUT,
            .serial = {
                    .baud = L_DEFAULT_MODBUS_BAUD_RATE,
                    .parity = L_DEFAULT_MODBUS_PARITY,
            },
    };

    if (unit_id > 0) {
        iface_param.server.unit_id = unit_id;
        // TODO: Set callbacks (if necessary)
    } else {
        iface_param.serial.stop_bits_client = L_DEFAULT_MODBUS_STOP_BITS_CLIENT;
    }

    int client_iface = modbus_iface_get_by_name(DEVICE_DT_NAME(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_modbus_serial)));
    int ret = modbus_init_client(client_iface, iface_param);
    if (ret < 0) {
        LOG_ERR("Failed to initialize Modbus: %d", ret);
    }

    return ret;
}

