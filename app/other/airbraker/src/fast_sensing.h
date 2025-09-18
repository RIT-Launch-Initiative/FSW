#include "flight.h"

int boost_and_flight_sensing(const struct device *imu_dev,
                             const struct device *barom_dev, const struct device *ina_servo);