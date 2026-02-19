#pragma once

#include "common.h"

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>

bool feed_boost_acc(k_ticks_t ts_ticks, const NTypes::AccelerometerData&);
bool feed_boost_barom(int64_t ts_us, float temp, float press);

