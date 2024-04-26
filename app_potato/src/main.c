#include <app_version.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/dev/uart.h>
#include <launch_core/extension_boards.h>
#include <launch_core/net/net_common.h>
#include <launch_core/net/udp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_potato);

// Threads
#define POTATO_STACK_SIZE (512)

static K_THREAD_STACK_DEFINE(adc_read_stack, POTATO_STACK_SIZE);
static struct k_thread adc_read_thread;

static K_THREAD_STACK_DEFINE(sensor_read_stack, POTATO_STACK_SIZE);
static struct k_thread sensor_read_thread;
// TODO: Might just be a process task that sends over SLIP, but also logs data
static K_THREAD_STACK_DEFINE(slip_tx_stack, POTATO_STACK_SIZE);
static struct k_thread slip_tx_thread;

// Queues
static K_QUEUE_DEFINE(slip_tx_queue);

static void adc_read_task(void *, void *, void *) {
    // Check ADC

    // Do ADC stuff
    while (1) {
    }
}

static void sensor_read_task(void *, void *, void *) {
    // Check devices

    // Do sensor stuff
    while (1) {
    }
}

static void slip_tx_task(void *, void *, void *) {
    while (1) {
    }
}

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

    // Initialize tasks
    // TODO: Maybe prioritize in this order (ADC, SLIP, sensors)
    k_thread_create(&adc_read_thread, &adc_read_stack[0], POTATO_STACK_SIZE, adc_read_task, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
    k_thread_create(&sensor_read_thread, &sensor_read_stack[0], POTATO_STACK_SIZE, sensor_read_task, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
    k_thread_create(&slip_tx_thread, &slip_tx_stack[0], POTATO_STACK_SIZE, slip_tx_task, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(10), 0, K_NO_WAIT);

    k_thread_start(&adc_read_thread);
    k_thread_start(&sensor_read_thread);
    k_thread_start(&slip_tx_thread);

    return 0;
}

int main() {
    init();

    return 0;
}
