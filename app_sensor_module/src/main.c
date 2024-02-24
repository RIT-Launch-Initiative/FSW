#include "sensors.h"
#include <launch_core/backplane_defs.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/net/net_common.h>
#include <launch_core/net/udp.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define STACK_SIZE (512)

LOG_MODULE_REGISTER(main, CONFIG_APP_SENSOR_MODULE_LOG_LEVEL);

#define STACK_SIZE (512)
#define TMP_UPDATE_TIME_MS (100)

// Queues
static struct k_msgq ten_hz_telemetry_queue;
static uint8_t ten_hz_telemetry_queue_buffer[CONFIG_TEN_HZ_QUEUE_SIZE * sizeof(ten_hz_telemetry_t)];

static struct k_msgq hundred_hz_telemetry_queue;
static uint8_t hundred_hz_telemetry_queue_buffer[CONFIG_HUNDRED_HZ_QUEUE_SIZE * sizeof(hundred_hz_telemetry_t)];

// Threads
static K_THREAD_STACK_DEFINE(telemetry_processing_stack, STACK_SIZE);
static struct k_thread telemetry_processing_thread;

// Devices
//#define LED0_NODE DT_ALIAS(led0)
//static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
//
//#define LED1_NODE DT_ALIAS(led1)
//static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

static void telemetry_processing_task(void *, void *, void *) {
    ten_hz_telemetry_t ten_hz_telem;
    hundred_hz_telemetry_t hundred_hz_telem;
    hundred_hz_telemetry_packed_t hundred_hz_telem_packed;

    while (true) {
        if (0 == k_msgq_get(&hundred_hz_telemetry_queue, &hundred_hz_telem, K_NO_WAIT)) {
            // TODO: Maybe make this a core lib function to handle copying data safely and avoid this mess
            hundred_hz_telem_packed.adxl375.accel_x = hundred_hz_telem.adxl375.accel_x;
            hundred_hz_telem_packed.adxl375.accel_y = hundred_hz_telem.adxl375.accel_y;
            hundred_hz_telem_packed.adxl375.accel_z = hundred_hz_telem.adxl375.accel_z;

            hundred_hz_telem_packed.lsm6dsl_accel.accel_x = hundred_hz_telem.lsm6dsl_accel.accel_x;
            hundred_hz_telem_packed.lsm6dsl_accel.accel_y = hundred_hz_telem.lsm6dsl_accel.accel_y;
            hundred_hz_telem_packed.lsm6dsl_accel.accel_z = hundred_hz_telem.lsm6dsl_accel.accel_z;

            hundred_hz_telem_packed.ms5611.pressure = hundred_hz_telem.ms5611.pressure;
            hundred_hz_telem_packed.ms5611.temperature = hundred_hz_telem.ms5611.temperature;

            hundred_hz_telem_packed.bmp388.pressure = hundred_hz_telem.bmp388.pressure;
            hundred_hz_telem_packed.bmp388.temperature = hundred_hz_telem.bmp388.temperature;

            hundred_hz_telem_packed.lsm6dsl_gyro.gyro_x = hundred_hz_telem.lsm6dsl_gyro.gyro_x;
            hundred_hz_telem_packed.lsm6dsl_gyro.gyro_y = hundred_hz_telem.lsm6dsl_gyro.gyro_y;
            hundred_hz_telem_packed.lsm6dsl_gyro.gyro_z = hundred_hz_telem.lsm6dsl_gyro.gyro_z;

            hundred_hz_telem_packed.lis3mdl.mag_x = hundred_hz_telem.lis3mdl.mag_x;
            hundred_hz_telem_packed.lis3mdl.mag_y = hundred_hz_telem.lis3mdl.mag_y;
            hundred_hz_telem_packed.lis3mdl.mag_z = hundred_hz_telem.lis3mdl.mag_z;

            l_send_udp_broadcast((uint8_t * ) & hundred_hz_telem_packed, sizeof(hundred_hz_telemetry_packed_t),
                                 SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT);
        } else {
            LOG_WRN("Failed to get data from 100 Hz queue");
        }

        if (0 == k_msgq_get(&ten_hz_telemetry_queue, &ten_hz_telem, K_NO_WAIT)) {
            l_send_udp_broadcast((uint8_t * ) & ten_hz_telem, sizeof(ten_hz_telemetry_t),
                                 SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_TEN_HZ_DATA_PORT);
        } else {
            LOG_WRN("Failed to get data from 10 Hz queue");
        }

        // TODO: Extension board support. Need to figure out a robust way of doing this

        // TODO: write to flash when data logging library is ready
    }
}

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
    int ret = 0;

    k_msgq_init(&ten_hz_telemetry_queue, ten_hz_telemetry_queue_buffer, sizeof(ten_hz_telemetry_t),
                CONFIG_TEN_HZ_QUEUE_SIZE);
    k_msgq_init(&hundred_hz_telemetry_queue, hundred_hz_telemetry_queue_buffer, sizeof(hundred_hz_telemetry_t),
                CONFIG_HUNDRED_HZ_QUEUE_SIZE);

    if (0 > l_create_ip_str_default_net_id(ip, SENSOR_MODULE_ID, 1)) {
        LOG_ERR("Failed to create IP address string: %d", ret);
        return -1;
    }

    if (!l_check_device(DEVICE_DT_GET_ONE(wiznet_w5500))) {
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

    while (1) {
    }

    return 0;
}


