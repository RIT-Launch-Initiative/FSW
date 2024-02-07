#include <app_version.h>

#include <launch_core/backplane_defs.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/dev/sensor.h>
#include <launch_core/net/net_common.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/storage/flash_map.h>

#include "sensors.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);
LOG_MODULE_REGISTER(tmp116_task, LOG_LEVEL_INF);
K_QUEUE_DEFINE(net_tx_queue);

#define STACK_SIZE (512)
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);

static void ina_task(void *, void *, void *) {
    struct sensor_value temperature;
    power_module_telemetry_t sensor_telemetry = {0};
    
    const struct device *tmp116_device = DEVICE_DT_GET_ANY(ti_tmp116);
    if (!device_is_ready(tmp116_device)) {
        LOG_ERR("whomp whomp TMP116 sensor not found");
        return;
    }

    // channels and storage for sensor readings
    enum sensor_channel channels[] = {SENSOR_CHAN_AMBIENT_TEMP};
    struct sensor_value *values[] = {&temperature};
    l_sensor_readings_args_t args = {
        .num_readings = 1,
        .channels = channels,
        .values = values, // Use if not converting to float
        .float_values = {&some_float_var}, // float values
    };
    while (true) {
        sensor_telemetry.timestamp = k_uptime_get_32();

        // Fetch and read temperature data from TMP116 using aaron's helper function
        int result = l_update_get_sensor_data(tmp116_device, &args, true); // set true if float conversion needed
        if (result == 0) {
            // If needed, convert sensor_value to float or double for processing
            float temp_celsius = sensor_value_to_double(&temperature);
            sensor_telemetry.data_temperature = temp_celsius;
        }

        // sleep for 100 ms
        k_sleep(K_MSEC(100));
    }
}

static int init(void) {
    char ip[MAX_IP_ADDRESS_STR_LEN];
    int ret = -1;

    k_queue_init(&net_tx_queue);

    if (0 > l_create_ip_str_default_net_id(ip, SENSOR_MODULE_ID, 1)) {
        LOG_ERR("Failed to create IP address string: %d", ret);
        return -1;
    }

    k_queue_init(&net_tx_queue);

    if (!l_check_device(wiznet)) {
        if (!l_init_udp_net_stack(ip)) {
            LOG_ERR("Failed to initialize network stack");
        }
    } else {
        LOG_ERR("Failed to get network device");
    }

    // Sensors

    return 0;
}

int main() {
    if (!init()) {
        return -1;
    }

    while (1) {
    }

    return 0;
}


