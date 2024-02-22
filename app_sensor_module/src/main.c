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
K_QUEUE_DEFINE(net_tx_queue);

#define STACK_SIZE (512)
#define TMP_UPDATE_TIME_MS (100)
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);

// Sensors
static K_THREAD_STACK_DEFINE(tmp_task_stack, STACK_SIZE);
static struct k_thread tmp_task_thread_data;

static void tmp_task(void *, void *, void *) {

    tmp116_telemetry_t sensor_telemetry = {0};
    
    const struct device *tmp116_device = DEVICE_DT_GET_ANY(ti_tmp116); // Get the TMP116 device
    if (!device_is_ready(tmp116_device)) {                             // check if ready
        LOG_ERR("TMP116 sensor not found");
        return;                                                        // device is not ready 
    }

    // Allocate mem for float storage
    float *float_storage = malloc(sizeof(float));

    // Define the sensor reading arguments
    l_sensor_readings_args_t args = {
        .num_readings = 1,
        .channels = (enum sensor_channel[]){SENSOR_CHAN_IR}, // change to channel
        .values = NULL, // Not used in this case
        .float_values = &float_storage
    };

    while (true){
        sensor_telemetry.timestamp = k_uptime_get_32();

        // Fetch and read temperature data from
        int result = l_update_get_sensor_data(tmp116_device, &args, true); // Set at true for float conversion
        if (result < 0){
            LOG_ERR("Failed to get tmp values");
        } else {
            // Log the reading
            LOG_INF("Temperature: %f C", *args.float_values[0]); // log the float value
        }

        // sleep
        uint32_t time_to_wait = TMP_UPDATE_TIME_MS - (k_uptime_get_32() - sensor_telemetry.timestamp);
        if (time_to_wait > 0) {
            k_sleep(K_MSEC(time_to_wait));
        }
    }
    // free allocated memory
    k_free(float_storage);
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

    const struct device *tmp116_device = DEVICE_DT_GET_ANY(ti_tmp116); // get device from tree
    // Check if TMP116 device exists
    if (!device_is_ready(tmp116_device)) {
        LOG_ERR("TMP116 sensor not found");
        return -1; // Return early if device is not found
    }
    
    // sensors
    k_thread_create(&tmp_task_thread_data, tmp_task_stack, STACK_SIZE,
                    tmp_task, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

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


