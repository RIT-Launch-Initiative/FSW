#pragma once

#include <zephyr/device.h>

#define PUMP_STOP_DUTY_CYCLE      0
#define PUMP_STOP_CURRENT         0
#define PUMP_DUTY_ON_MS           1000
#define PUMP_DUTY_OFF_MS          1000
#define PUMP_DUTY_OFF_MS_INITITAL 1000
#define PUMP_TIME_END             2
#define PUMP_CURRENT_END 2

// Blocks for max of PUMP_DUTY_ON_MS
int attempt_inflation_iteration(const struct device *ina_pump);