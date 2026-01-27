#include <zephyr/device.h>



#define PUMP_CURRENT_END 2
// Blocks for max of PUMP_DUTY_ON_MS
int attempt_inflation_iteration(const struct device *ina_pump);