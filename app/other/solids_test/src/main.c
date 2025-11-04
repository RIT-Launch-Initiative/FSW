#include "adc_reading.h"
#include "buzzer.h"
#include "button.h"
#include "config.h"

#include <zephyr/logging/log.h>
#include <zephyr/init.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// APPLICATION: Executed just before application code (main)
SYS_INIT(adc_init, APPLICATION, SYS_INIT_PRIORITY);
SYS_INIT(buzzer_init, APPLICATION, SYS_INIT_PRIORITY);
SYS_INIT(button_init, APPLICATION, SYS_INIT_PRIORITY);

int main(void) {
	LOG_INF("Solids Test Start");
	
	LOG_INF("Use 'test start' to begin test");
    LOG_INF("Commands: test start | test stop | test dump [optional test #] | test erase | test read [optional #]");

	return 0;
}