#include <launch_core_classic/dev/uart.h>

LOG_MODULE_REGISTER(launch_uart_utils);

int l_uart_init_rs485(const struct device *dev) {
    struct uart_config config = {0};

    int ret = uart_config_get(dev, &config);
    if (unlikely(ret < 0)) {
        LOG_ERR("Failed to get UART device configuration");
        return ret;
    }

    config.flow_ctrl = UART_CFG_FLOW_CTRL_RS485;

    ret = uart_configure(dev, &config);
    if (unlikely(ret < 0)) {
        LOG_ERR("Failed to set UART device configuration with RS485 flow control");
    }

    return ret;
}
