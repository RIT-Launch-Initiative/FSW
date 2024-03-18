#include <launch_core/dev/uart.h>

LOG_MODULE_REGISTER(launch_uart_utils);

int l_uart_init_rs485(const struct device *dev) {
    struct uart_config config = {0};

    int ret = uart_config_get(dev, &config);
    if (unlikely(ret < 0)) {
        LOG_INF()
    }


    return uart_configure(dev, &config);
}
