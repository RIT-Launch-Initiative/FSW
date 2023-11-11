/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <app_version.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>



#define SLEEP_TIME_MS   1000

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

// Check if board is power module
//#ifdef CONFIG_BOARD_POWER_MODULE
//
//int main(void) {
//    const struct device *const ina = DEVICE_DT_GET_ONE(ti_ina219);
//    struct sensor_value v_bus;
//    struct sensor_value power;
//    struct sensor_value current;
//
//    if (!device_is_ready(ina)) {
//        printf("Device %s is not ready.\n", ina->name);
//        return 0;
//    }
//
//    while (true) {
//        if (sensor_sample_fetch(ina)) {
//            printf("Could not fetch sensor data.\n");
//            return 0;
//        }
//
//        sensor_channel_get(ina, SENSOR_CHAN_VOLTAGE, &v_bus);
//        sensor_channel_get(ina, SENSOR_CHAN_POWER, &power);
//        sensor_channel_get(ina, SENSOR_CHAN_CURRENT, &current);
//
//        printf("Bus: %f [V] -- "
//               "Power: %f [W] -- "
//               "Current: %f [A]\n",
//               sensor_value_to_double(&v_bus),
//               sensor_value_to_double(&power),
//               sensor_value_to_double(&current));
//        k_sleep(K_MSEC(2000));
//    }
//
//    return 0;
//}
//
//#elif CONFIG_BOARD_RADIO_MODULE

K_QUEUE_DEFINE(lora_tx_queue);
K_QUEUE_DEFINE(net_tx_queue);

static void network_init() {

}


static void init(void) {
    // Queues
    k_queue_init(&lora_tx_queue);
    k_queue_init(&net_tx_queue);

    // Devices
    const struct device *const sx1276 = DEVICE_DT_GET_ONE(semtech_sx1276);
    if (!device_is_ready(sx1276)) {
        printk("Device %s is not ready.\n", sx1276->name);
    }

    const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);
    if (!device_is_ready(wiznet)) {
        printk("Device %s is not ready.\n", wiznet->name);
    }

    network_init();
}

static void get_gnss(void) {

}


static void lora_tx() {
    if (!k_queue_is_empty(&lora_tx_queue)) {


    }
}

static void wiznet_tx() {

}

int main() {
    init();




    return 0;
}


//#elif CONFIG_BOARD_SENSOR_MODULE
//int main() {
//
//    return 0;
//}
//
//
//#else
//
//#endif
