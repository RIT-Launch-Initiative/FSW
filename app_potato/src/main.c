#include <launch_core/dev/dev_common.h>
#include <launch_core/dev/uart.h>
#include <launch_core/extension_boards.h>
#include <launch_core/net/net_common.h>
#include <launch_core/net/udp.h>
#include <launch_core/os/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_potato);

// Queues
static K_QUEUE_DEFINE(slip_tx_queue);

// Extern Variables
bool logging_enabled = false;


int init_slip_network(void) {
    char rs485_ip[MAX_IP_ADDRESS_STR_LEN];

    int ret = l_uart_init_rs485(DEVICE_DT_GET(DT_NODELABEL(uart5))) != 0;
    if (ret != 0) {
        LOG_ERR("Failed to initialize UART to RS485");
        return ret;
    }

    ret = l_create_ip_str(rs485_ip, 11, 1, 2, 1) != 0;
    if (ret != 0) {
        LOG_ERR("Failed to create IP address string");
        return ret;
    }

    ret = l_init_udp_net_stack_by_device(DEVICE_DT_GET(DT_NODELABEL(uart5)), rs485_ip);
    if (ret != 0) {
        LOG_ERR("Failed to initialize network stack");
    }

    return ret;
}

static int init(void) {
    if (init_slip_network() == 0) {
        // Arbitrate with connected module over SLIP
        initiate_arbitration(POTATO_EXTENSION_BOARD_ID, 0);
    }
    return 0;
}

int main() {
    init();
    l_fs_boot_count_check();

    return 0;
}
