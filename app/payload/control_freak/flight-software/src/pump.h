#include <zephyr/device.h>

#define PUMP_STOP_DUTY_CYCLE 0
#define PUMP_STOP_CURRENT    1

int attempt_inflation_iteration(const struct device *ina_pump);