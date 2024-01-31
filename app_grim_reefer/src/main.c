/*
 * Copyright (c) 2023 Richie Sommers
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

static int init(void) { return 0; }

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
