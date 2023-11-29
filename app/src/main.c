#include <zephyr/kernel.h>
#include <app_version.h>
#include <zephyr/device.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/sensor/tmp116.h>

#include <zephyr/logging/log.h>

#include <zephyr/net/socket.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>

#define SLEEP_TIME_MS   1000

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);


int main(void) {
	return 0;
}
