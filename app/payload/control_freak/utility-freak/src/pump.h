#include <zephyr/device.h>
#define PUMP_DUTY_ON_MS  10000
#define PUMP_DUTY_OFF_MS 20000

int attempt_inflation_iteration(const struct device *ina_pump);