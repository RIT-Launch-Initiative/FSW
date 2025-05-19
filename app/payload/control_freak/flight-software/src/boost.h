#ifndef CONTROL_FREAK_BOOST_H
#define CONTROL_FREAK_BOOST_H

#include <stdbool.h>
#include <stdint.h>

bool feed_boost_acc(int64_t ts_us, float *xyz);
bool feed_boost_barom(int64_t ts_us, float temp, float press);

#endif