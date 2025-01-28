#include "zephyr/logging/log.h"

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
LOG_MODULE_REGISTER(main);

#include "rfm9Xw.h"

#include <zephyr/drivers/rtc.h>

const struct device *radio_dev = DEVICE_DT_GET(DT_NODELABEL(radio));
// Needed for Horus Packet
const struct device *const rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

int main(void) {

    while (1) {

        k_msleep(1000);
    }
    return 0;
}
