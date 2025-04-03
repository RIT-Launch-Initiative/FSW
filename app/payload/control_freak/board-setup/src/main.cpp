#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define GPSRST_NODE DT_ALIAS(gpsreset)
static const struct gpio_dt_spec gpsreset = GPIO_DT_SPEC_GET(GPSRST_NODE, gpios);

#define GPSSAFE_NODE DT_ALIAS(gpssafeboot)
static const struct gpio_dt_spec gpssafeboot = GPIO_DT_SPEC_GET(GPSSAFE_NODE, gpios);

int main() {
    int ret;
    bool led_state = true;

    if (!gpio_is_ready_dt(&led)) {
        printk("No LED :(\n");
        return 0;
    }

    if (!gpio_is_ready_dt(&gpsreset)) {
        printk("No GPS RST :(\n");
        return 0;
    }
    if (!gpio_is_ready_dt(&gpssafeboot)) {
        printk("No GPS safe :(\n");
        return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Failed to conf led :(\n");
        return 0;
    }
    ret = gpio_pin_configure_dt(&gpsreset, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Failed to conf gps reset :(\n");
        return 0;
    }

    ret = gpio_pin_configure_dt(&gpssafeboot, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Failed to conf gps safe :(\n");
        return 0;
    }

    ret = gpio_pin_set_dt(&gpssafeboot, 0);
    if (ret < 0) {
        printk("couldnt set gpssafeboot: %d", ret);
    }

    k_msleep(1);

    // RESET GPS

    ret = gpio_pin_set_dt(&gpsreset, 1);
    if (ret < 0) {
        printk("couldnt set gpsreset: %d", ret);
    }
    k_msleep(2);
    ret = gpio_pin_set_dt(&gpsreset, 0);
    if (ret < 0) {
        printk("couldnt set gpsreset: %d", ret);
    }
    printk("Reset GPS\n");

    // ret = gpio_pin_configure_dt(&gpssafeboot, GPIO_INPUT);
    // if (ret < 0) {
    // printk("Failed to conf gps safe back to input:(\n");
    // return 0;
    // }
    // printf("As timepulse");

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