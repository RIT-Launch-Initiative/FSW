#include <launch_core/backplane_defs.h>
#include <launch_core/types.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/dev/sensor.h>
#include <launch_core/net/net_common.h>
#include <launch_core/dev/uart.h>
#include <launch_core/net/udp.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define SENSOR_MODULE_IP_ADDR BACKPLANE_IP(SENSOR_MODULE_ID, 2, 1) // TODO: KConfig the board revision and #
#define TMP_UPDATE_TIME_MS (100)
#define STACK_SIZE (512)

LOG_MODULE_REGISTER(main, CONFIG_APP_SENSOR_MODULE_LOG_LEVEL);

// Queues
static struct k_msgq ten_hz_telemetry_queue;
static uint8_t ten_hz_telemetry_queue_buffer[CONFIG_TEN_HZ_QUEUE_SIZE * sizeof(sensor_module_ten_hz_telemetry_t)];

static struct k_msgq hundred_hz_telemetry_queue;
static uint8_t hundred_hz_telemetry_queue_buffer[
        CONFIG_HUNDRED_HZ_QUEUE_SIZE * sizeof(sensor_module_hundred_hz_telemetry_t)];

// Threads
static K_THREAD_STACK_DEFINE(telemetry_processing_stack, STACK_SIZE);
static struct k_thread telemetry_processing_thread;

// Devices
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

static void telemetry_processing_task(void *, void *, void *) {
    sensor_module_ten_hz_telemetry_t ten_hz_telem;
    sensor_module_hundred_hz_telemetry_t hundred_hz_telem;
    
    int ten_hz_socket = l_init_udp_socket(SENSOR_MODULE_IP_ADDR,
                                          SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_TEN_HZ_DATA_PORT);
    int hundred_hz_socket = l_init_udp_socket(SENSOR_MODULE_IP_ADDR,
                                              SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT);

    while (true) {
if (0 == k_msgq_get(&ten_hz_telemetry_queue, &ten_hz_telem, K_NO_WAIT)) {
            l_send_udp_broadcast(ten_hz_socket, (uint8_t *) &ten_hz_telem, sizeof(sensor_module_ten_hz_telemetry_t),
                                 SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_TEN_HZ_DATA_PORT);
        } else {
            LOG_WRN("Failed to get data from 10 Hz queue");
        }

        if (0 == k_msgq_get(&hundred_hz_telemetry_queue, &hundred_hz_telem, K_NO_WAIT)) {
            l_send_udp_broadcast(hundred_hz_socket, (uint8_t *) &hundred_hz_telem,
                                 sizeof(sensor_module_hundred_hz_telemetry_t),
                                 SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT);
        } else {
            LOG_WRN("Failed to get data from 100 Hz queue");
        }

        // TODO: Extension board support. Need to figure out a robust way of doing this

        // TODO: write to flash when data logging library is ready
    }
}

// Sensors
static K_THREAD_STACK_DEFINE(tmp_task_stack, STACK_SIZE);
static struct k_thread tmp_task_thread_data;

static void tmp_task(void *, void *, void *) {

    const struct device *tmp116_device = DEVICE_DT_GET_ANY(ti_tmp116);
    if (!device_is_ready(tmp116_device)) {                             // check if ready
        LOG_ERR("TMP116 sensor not found");
        return;                                                        // device is not ready 
    }

    enum sensor_channel channel = SENSOR_CHAN_AMBIENT_TEMP;
    l_temperature_data_t temperature = {0};
    struct sensor_value temp_sensor_value = {0};
    struct sensor_value *p_temp_sensor_value = &temp_sensor_value;


    while (true) {
        uint32_t timestamp = k_uptime_get_32();

        int result = l_get_sensor_data(tmp116_device, 1, &channel, &p_temp_sensor_value);
        // TODO: Fill out temperature float

        if (result < 0) {
            LOG_ERR("Failed to get tmp values");
        } else {
            temperature = sensor_value_to_double(p_temp_sensor_value);
            LOG_INF("Temperature: %f C", temperature);
        }

        // sleep
        uint32_t time_to_wait = TMP_UPDATE_TIME_MS - (k_uptime_get_32() - timestamp);
        if (time_to_wait > 0) {
            k_sleep(K_MSEC(time_to_wait));
        }
    }
}


static void initialize_networks(void) {
    char eth_ip[MAX_IP_ADDRESS_STR_LEN];
    char rs485_ip[MAX_IP_ADDRESS_STR_LEN];

    if (!l_check_device(DEVICE_DT_GET_ONE(wiznet_w5500))) {
        if (l_create_ip_str(eth_ip, 10, 3, 2, 1) == 0) {
            if (!l_init_udp_net_stack_default(eth_ip)) {
            LOG_ERR("Failed to initialize network stack");
}
        } else {
            LOG_ERR("Failed to create IP address string");
        }
    } else {
        LOG_ERR("Failed to get network device");
    }

    if (l_uart_init_rs485(DEVICE_DT_GET(DT_NODELABEL(uart5))) != 0) {
        if (l_create_ip_str(rs485_ip, 11, 0, 3, 1) == 0) {
            if (l_init_udp_net_stack_by_device(DEVICE_DT_GET(DT_NODELABEL(uart5)), rs485_ip)) {
                LOG_ERR("Failed to initialize network stack");
            }
        } else {
            LOG_ERR("Failed to create IP address string");
        }
    } else {
        LOG_ERR("Failed to initialize UART to RS485");;
    }
}



static int init(void) {
    k_msgq_init(&ten_hz_telemetry_queue, ten_hz_telemetry_queue_buffer, sizeof(sensor_module_ten_hz_telemetry_t),
                CONFIG_TEN_HZ_QUEUE_SIZE);
    k_msgq_init(&hundred_hz_telemetry_queue, hundred_hz_telemetry_queue_buffer,
                sizeof(sensor_module_hundred_hz_telemetry_t),
                CONFIG_HUNDRED_HZ_QUEUE_SIZE);


    initialize_networks();

    const struct device *tmp116_device = DEVICE_DT_GET_ANY(ti_tmp116); // get device from tree
    // TODO: Remove and put in task Brian
    if (!device_is_ready(tmp116_device)) {
        LOG_ERR("TMP116 sensor not found");
        return -1; // Return early if device is not found
    }

    // Tasks
    k_thread_create(&tmp_task_thread_data, tmp_task_stack, STACK_SIZE,
                    tmp_task, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

    k_thread_create(&telemetry_processing_thread, &telemetry_processing_stack[0], STACK_SIZE,
                    telemetry_processing_task, NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
    k_thread_start(&telemetry_processing_thread);


    return 0;
}


int main() {
    if (!init()) {
        return -1;
    }

    return 0;
}


