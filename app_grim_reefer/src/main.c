/*
 * Copyright (c) 2023 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
// #include <zephyr/fs/fs.h>
// #include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_GRIM_REEFER_LOG_LEVEL_DBG);

// devicetree gets
#define LED1_NODE DT_NODELABEL(redled)
#define LED2_NODE DT_NODELABEL(anotherled)

const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

#define LDO_EN_NODE DT_NODELABEL(ldo_enable)
#define CAM_EN_NODE DT_NODELABEL(cam_enable)

const struct gpio_dt_spec ldo_enable = GPIO_DT_SPEC_GET(LDO_EN_NODE, gpios);
const struct gpio_dt_spec cam_enable = GPIO_DT_SPEC_GET(CAM_EN_NODE, gpios);

static int init(void) {
  // Init LEDS
  if (!gpio_is_ready_dt(&led1)) {
    LOG_ERR("LED 1 is not ready\n");
    return -1;
  }
  if (gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE) < 0) {
    LOG_ERR("Unable to configure LED 1 output pin\n");
    return -1;
  }
  if (!gpio_is_ready_dt(&led2)) {
    LOG_ERR("LED 2 is not ready\n");
    return -1;
  }
  if (gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE) < 0) {
    LOG_ERR("Unable to configure LED 2 output pin\n");
    return -1;
  }
  // Init Enable pins
  if (!gpio_is_ready_dt(&ldo_enable)) {
    LOG_ERR("ldo enable pin is not ready\n");
    return -1;
  }
  if (gpio_pin_configure_dt(&ldo_enable, GPIO_OUTPUT_ACTIVE) < 0) {
    LOG_ERR("Unable to configure ldo enable output pin\n");
    return -1;
  }
  if (!gpio_is_ready_dt(&cam_enable)) {
    LOG_ERR("camera enable pin is not ready\n");
    return -1;
  }
  if (gpio_pin_configure_dt(&cam_enable, GPIO_OUTPUT_ACTIVE) < 0) {
    LOG_ERR("Unable to configure camera enable output pin\n");
    return -1;
  }
  return 0;
}

int main(void) {
  if (init()) {
    return -1;
  }

  // Won't run if initializing the network stack failed
  while (true) {
    // convert_and_send();

    k_sleep(K_MSEC(100));
  }
  return 0;
}
