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
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, ldo_enable);

int main(void) {
  int counter = 0;
  // Boilerplate: set up GPIO
  if (!gpio_is_ready_dt(&led)) {
    LOG_ERR("GPIO is not ready\n");
    return -1;
  }

  if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) {
    LOG_ERR("Unable to configure LED output pin\n");
    return -1;
  }
  if (!gpio_is_ready_dt(&ldo_enable)) {
    LOG_ERR("LDO Enable GPIO is not ready\n");
    return 0;
  }

  if (gpio_pin_configure_dt(&ldo_enable, GPIO_OUTPUT_ACTIVE) < 0) {
    LOG_ERR("Unable to configure LDO output pin\n");
    return -1;
  }
  /* Configure the pin */

  // forever
  while (1) {
    gpio_pin_toggle_dt(&led);
    LOG_PRINTK("LED toggle %d\n", ++counter);
    k_msleep(100);
  }
  return 0;
}
