#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main() {
    int ret;
    bool led_state = true;

    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }

    while (1) {
        // ret = gpio_pin_toggle_dt(&led);
        // if (ret < 0) {
        // return 0;
        // }
        //
        // led_state = !led_state;
        // printf("LED state: %s\n", led_state ? "ON" : "OFF");
        k_msleep(20000);
    }
    return 0;
}