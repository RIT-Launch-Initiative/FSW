#include "data_storage.h"
#include <stdint.h>

// read data without filling in timestamp
// 3 x INA260
// 1 x Temp
// 1 x Humidity
int read_slow(struct slow_data *dat);
int read_fast(struct fast_data *dat);
int read_adc(struct adc_data *dat);