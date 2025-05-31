#include "data.h"
int slow_sensing_thread(void *, void *, void *);

// submit data from i2c thread to the sensor
int submit_slowdata(NTypes::AccelerometerData &imu, float &tempC, float &current, float &voltage);