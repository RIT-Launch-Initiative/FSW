#include "common.h"
int slow_sensing_thread(void *, void *, void *);

// submit data from i2c thread to the sensor
int submit_slowdata(const small_orientation &orient, const float &tempC, const float &current, const float &voltage,
                    FlipState flip_state, FlightState flight_state);