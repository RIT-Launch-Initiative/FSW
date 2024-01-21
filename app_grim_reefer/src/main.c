/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include "zephyr/device.h"
#include "zephyr/devicetree.h"
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/sensor.h>  

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

static int init(void) {
    return 0;
}
// Did you break device tree
#if !DT_NODE_EXISTS(DT_ALIAS(my_adc))
    #error "whoops"
#endif

/*
 * Get a device structure from a devicetree node with compatible
 * "bosch,bme280". (If there are multiple, just pick one.)
 */
static const struct device *get_adc(void)
{
    
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

