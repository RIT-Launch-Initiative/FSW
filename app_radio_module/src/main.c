#include <zephyr/kernel.h>
#include <app_version.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/console/console.h>

#include <zephyr/random/random.h>

#include "net_utils.h"
#include "lora_utils.h"


#define SLEEP_TIME_MS   100

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

extern const struct device *const lora_dev;

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);
K_QUEUE_DEFINE(lora_tx_queue);
K_QUEUE_DEFINE(net_tx_queue);


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
        init_net_stack();
    }
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
        &data->temperature_tmp
    };

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
        lora_tx(lora_dev, (uint8_t *) &data, sizeof(FAKE_SENSOR_DATA_T));
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
