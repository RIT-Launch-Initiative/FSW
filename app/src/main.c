/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <app_version.h>
#include <zephyr/logging/log.h>

#define SLEEP_TIME_MS   1000
#define LED0_NODE DT_ALIAS(led0)

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL
);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

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
int main() {
    int ret;

    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }

    while (1) {
        printk("Hello World!\n");
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            return 0;
        }
        k_msleep(SLEEP_TIME_MS);
    }

    while (1) {
    }

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
