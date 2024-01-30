#include "zephyr/logging/log.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

#include <stdio.h>
#include <zephyr/drivers/gpio.h>

// devicetree gets
#define LED_NODE DT_ALIAS(led)

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

const struct gpio_dt_spec ldo_enable =
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, gpio_enable_ldos);
const struct gpio_dt_spec cam_enable =
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, gpio_enable_cameras);

int main(void) {
  int counter = 0;
  // Boilerplate: set up GPIO
  if (!gpio_is_ready_dt(&led)) {
    printf("GPIO is not ready\n");
    return 0;
  }

  if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) {
    printf("Unable to configure LED output pin\n");
    return 0;
  }

  // forever
  while (1) {
    gpio_pin_toggle_dt(&led);
    printk("LED toggle %d\n", ++counter);
    k_msleep(100);
  }
  return 0;
}
