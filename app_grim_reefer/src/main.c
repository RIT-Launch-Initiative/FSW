/*
 * Copyright (c) 2023 Richie Sommers
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/device.h"
#include "zephyr/devicetree.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

#ifdef BOARD_GRIM_REEFER
LOG_MODULE_REGISTER(main, CONFIG_APP_GRIM_REEFER_LOG_LEVEL_DBG);
#else
#define CONFIG_APP_GRIM_REEFER_LOG_LEVEL_DBG 1
LOG_MODULE_REGISTER(main, CONFIG_APP_GRIM_REEFER_LOG_LEVEL_DBG);
#endif

// devicetree gets
#define LED1_NODE DT_NODELABEL(redled)
#define LED2_NODE DT_NODELABEL(anotherled)

const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

#define LDO_EN_NODE DT_NODELABEL(ldo_enable)
#define CAM_EN_NODE DT_NODELABEL(cam_enable)

const struct gpio_dt_spec ldo_enable = GPIO_DT_SPEC_GET(LDO_EN_NODE, gpios);
const struct gpio_dt_spec cam_enable = GPIO_DT_SPEC_GET(CAM_EN_NODE, gpios);

#define FLASH_NODE DT_ALIAS(storage)
const struct device *const flash = DEVICE_DT_GET(FLASH_NODE);

static int init(void) { return 0; }
// Did you break device tree
#if !DT_NODE_EXISTS(DT_ALIAS(my_adc))
#error "whoops"
#endif

#if DT_NUM_INST_STATUS_OKAY(microchip_mcp3561) == 0
#error "mcp3561 adc not in DTS"
#else
// #error microchip_mcp3561 has some DT_NUM_INST_STATUS_OKAY(microchip_mcp3561)
#endif

/*
 * Get a device structure from a devicetree node with compatible
 * "bosch,bme280". (If there are multiple, just pick one.)
 */
static const struct device *get_adc(void) {

  const struct device *const dev = DEVICE_DT_GET_ONE(microchip_mcp3561);

  if (dev == NULL) {
    /* No such node, or the node does not have status "okay". */
    printk("\nError: no device found.\n");
    return NULL;
  }

  if (!device_is_ready(dev)) {
    printk("\nError: Device \"%s\" is not ready; "
           "check the driver initialization logs for errors.\n",
           dev->name);
    return NULL;
  }

  printk("Found device \"%s\", getting sensor data\n", dev->name);
  return dev;
}

int main(void) {
  if (init()) {
    return -1;
  }

  const struct device *dev = get_adc();

  if (dev == NULL) {
    return 0;
  }

  struct sensor_value volts;
  // Won't run if initializing the network stack failed
  while (true) {
    sensor_sample_fetch(dev);
    sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE, &volts);

    k_sleep(K_MSEC(100));
  }
  return 0;
}
