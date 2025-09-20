#include <zephyr/device.h>

#define PUMP_STOP_DUTY_CYCLE 0
#define PUMP_STOP_CURRENT    1

#define PUMP_DUTY_ON_MS  10000
#define PUMP_DUTY_OFF_MS 20000
#define PUMP_DUTY_OFF_MS_INITITAL 5000

#define PUMP_TIME_END    1
#define PUMP_CURRENT_END 2
// Blocks for max of PUMP_DUTY_ON_MS
int attempt_inflation_iteration(const struct device *ina_pump);