#include "flight.h"

int flight_sensing(const struct device *imu_dev, const struct device *barom_dev,
                   FreakFlightController *freak_controller);
