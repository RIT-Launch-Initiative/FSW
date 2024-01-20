#include <app_version.h>
#include <zephyr/console/console.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/random.h>

#include "lora_utils.h"
#include "net_utils.h"
#include "ubxlib_utils.h"

#define SLEEP_TIME_MS 100

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

extern const struct device *const lora_dev;

gnss_dev_t *gnss_dev;
extern int start_maxm10s(gnss_dev_t *dev);

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);
K_QUEUE_DEFINE(lora_tx_queue);
K_QUEUE_DEFINE(net_tx_queue);

// GNSS init thread
static void gnss_init_task(void) {
    printk("Initializing GNSS...\n");
    int ret = start_maxm10s(gnss_dev);
    if (!ret)
        printk("GNSS initialized\n");
    else
        printk("Error initializing GNSS. Got %d", ret);
}
#define GNSS_INIT_STACK_SIZE 2 << 10
#define GNSS_INIT_PRIORITY 4
K_THREAD_STACK_DEFINE(gnss_init_stack_area, GNSS_INIT_STACK_SIZE);
static struct k_thread gnss_init_thread_data;
static k_tid_t gnss_init_tid;

// init method
static void init() {
    if (!init_sx1276(lora_dev)) {
        int ret = lora_configure(lora_dev, false);
        if (ret != 0) {
            printk("Error initializing LORA device. Got %d", ret);
        } else {
            printk("LoRa configured\n");
        }
    }

    if (!init_eth_iface()) {
        int ret = init_net_stack();
        if (ret != 0) {
            printk("Error initializing network stack. Got %d", ret);
        } else {
            printk("Network stack initialized\n");
        }
    }

    // start gnss init thread
    gnss_init_tid = k_thread_create(&gnss_init_thread_data, gnss_init_stack_area,
                                    K_THREAD_STACK_SIZEOF(gnss_init_stack_area),
                                    gnss_init_task, NULL, NULL, NULL,
                                    GNSS_INIT_PRIORITY, 0, K_NO_WAIT);
}

static void initialize_fake_sensor_data(FAKE_SENSOR_DATA_T *data) {
    // Initialize port with a random value
    sys_rand_get(&data->port, sizeof(data->port));

    // Initialize floating-point values with random data
    float *float_fields[] = {
        &data->pressure_ms5, &data->temperature_ms5,
        &data->pressure_bmp3, &data->temperature_bmp3,
        &data->accel_x, &data->accel_y, &data->accel_z,
        &data->magn_x, &data->magn_y, &data->magn_z,
        &data->gyro_x, &data->gyro_y, &data->gyro_z,
        &data->temperature_tmp};

    for (size_t i = 0; i < sizeof(float_fields) / sizeof(float *); ++i) {
        uint32_t random_value;
        sys_rand_get(&random_value, sizeof(random_value));
        float rand_float = (float)(random_value % 100);
        *(float_fields[i]) = rand_float;
    }
}

int main() {
    const struct device *uart_dev = DEVICE_DT_GET(DT_ALIAS(dbguart));

    uint8_t tx_buff[255] = {0};
    uint8_t tx_buff_len = 0;

    printk("Starting radio module!\n");
    init();

    while (1) {
        FAKE_SENSOR_DATA_T data;
        initialize_fake_sensor_data(&data);

        data.port = 11000;
        lora_tx(lora_dev, (uint8_t *)&data, sizeof(FAKE_SENSOR_DATA_T));
        gpio_pin_toggle_dt(&led0);
        k_msleep(100);
    }

    return 0;
}

// int main() {
//     init();
//     printk("Receiver started\n");
//     while (1) {
//         int ret = lora_recv_async(lora_dev, lora_debug_recv_cb);
//     }
//
//     return 0;
// }
