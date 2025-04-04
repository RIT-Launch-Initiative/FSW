#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define GPSRST_NODE DT_ALIAS(gpsreset)
static const struct gpio_dt_spec gpsreset = GPIO_DT_SPEC_GET(GPSRST_NODE, gpios);

#define GPSSAFE_NODE DT_ALIAS(gpssafeboot)
static const struct gpio_dt_spec gpssafeboot = GPIO_DT_SPEC_GET(GPSSAFE_NODE, gpios);

int main() {
    int ret;
    if (!gpio_is_ready_dt(&gpsreset)) {
        printk("No GPS RST :(\n");
        return 0;
    }
    if (!gpio_is_ready_dt(&gpssafeboot)) {
        printk("No GPS safe :(\n");
        return 0;
    }

    ret = gpio_pin_configure_dt(&gpsreset, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Failed to conf gps reset :(\n");
        return 0;
    }

    // Safeboot active low (send downwards before reset to enter safeboot)
    ret = gpio_pin_configure_dt(&gpssafeboot, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Failed to conf gps safe :(\n");
        return 0;
    }
    // Don't enter safeboot: pin to logic 0
    ret = gpio_pin_set_dt(&gpssafeboot, 0);
    if (ret < 0) {
        printk("couldnt set gpssafeboot: %d", ret);
    }

    k_msleep(1);

    // Gps Reset Routine

    ret = gpio_pin_set_dt(&gpsreset, 1);
    if (ret < 0) {
        printk("couldnt set gpsreset: %d", ret);
    }
    k_msleep(2);
    ret = gpio_pin_set_dt(&gpsreset, 0);
    if (ret < 0) {
        printk("couldnt set gpsreset: %d", ret);
    }
    printk("GPS Reset\n");

    return 0;
}
