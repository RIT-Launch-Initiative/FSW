#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(sensor_mod, CONFIG_LOG_DEFAULT_LEVEL);

/* External declarations from other files */
extern void sensorTaskInit(void);
extern void shellCommandsInit(void);

int main() {
    sensorTaskInit();
    shellCommandsInit();

    LOG_INF("All subsystems initialized");
}
