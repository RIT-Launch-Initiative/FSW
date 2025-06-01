#include "common.h"
int slow_sensing_thread(void *, void *, void *);

// submit data from i2c thread to the sensor
int submit_slowdata(const NTypes::AccelerometerData &normed, const float &tempC, const float &current,
                    const float &voltage, FlipState flip_state);