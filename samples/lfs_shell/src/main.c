#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <zephyr/drivers/flash.h>

// NOTE: Hardware prerequisites
// 	Launch Mikroe Click Shield
// 	W25Q128JV breakout connected to port 1

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(app);

int main(void) {
	return 0;
}
