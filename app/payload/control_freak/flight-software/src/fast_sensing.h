#include "flight.h"

int boost_and_flight_sensing(const struct device *superfast_storage, const struct device *imu_dev,
                             const struct device *barom_dev, FreakFlightController *freak_controller);
