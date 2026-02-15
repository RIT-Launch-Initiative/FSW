#include "adc_reading.h"
#include "flash_storage.h"
#include "buzzer.h"
#include "button.h"
#include "config.h"

#include <zephyr/logging/log.h>
#include <zephyr/init.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// APPLICATION: Executed just before application code (main)
SYS_INIT(adc_init, APPLICATION, SYS_INIT_PRIORITY);
SYS_INIT(buzzer_init, APPLICATION, SYS_INIT_PRIORITY);
SYS_INIT(button_switch_init, APPLICATION, SYS_INIT_PRIORITY);

K_THREAD_DEFINE(adc_thread, 1024, adc_reading_task, NULL, NULL, NULL, 15, 0, THREAD_START_DELAY);
K_THREAD_DEFINE(buzz_thread, 512, buzzer_task, NULL, NULL, NULL, 10, 0, 0);
K_THREAD_DEFINE(storage_thread, 2048, flash_storage_thread_entry, NULL, NULL, NULL, STORAGE_THREAD_PRIORITY, 0, 1000);

int main(void) {
	LOG_INF("Solids Test Start");
	LOG_INF("Use 'test start [calibration name]' to begin test");
    LOG_INF("Use 'test help' to see all available commands");
	return 0;
}