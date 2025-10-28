#include "control.h"
#include "buzzer.h"
#include "button.h"

#include <zephyr/logging/log.h>
#include <zephyr/init.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

SYS_INIT(control_init, APPLICATION, 0);
SYS_INIT(buzzer_init, APPLICATION, 0);
SYS_INIT(button_init, APPLICATION, 0);

int main (void) {
	LOG_INF("Solids Test Start");
	
	LOG_INF("Use 'test start' to begin test");
    LOG_INF("Commands: test start | test stop | test dump [optional test #] | test erase | test read [optional #]");

	return 0;
}