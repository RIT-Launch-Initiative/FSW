#include "control.h"
#include "buzzer.h"
#include "button.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main (void) {
	LOG_INF("Solids Test Start");

	control_init();
	buzzer_init();
	button_init();
	
	LOG_INF("Use 'test start' to begin test");
    LOG_INF("Commands: test start | test stop | test dump");

	return 0;
}