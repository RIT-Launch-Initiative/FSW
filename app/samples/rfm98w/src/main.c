#include "zephyr/logging/log.h"

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
LOG_MODULE_REGISTER(main);

#include "rfm9Xw.h"

const struct device *radio_dev = DEVICE_DT_GET(DT_NODELABEL(radio));

#define LED_NODE DT_ALIAS(led)
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

int main(void) {
    if (!gpio_is_ready_dt(&led)) {
        printf("GPIO is not ready\n");
        return 0;
    }

    if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) {
        printf("Unable to configure LED output pin\n");
        return 0;
    }

    int counter = 0;
    int err = rfm9x_dostuff(radio_dev);
    if (err != 0) {
        LOG_ERR("RFM Returned %d", err);
    }
    while (1) {

        LOG_INF("Tick %d", ++counter);
        k_msleep(5000);
    }
    return 0;
}
