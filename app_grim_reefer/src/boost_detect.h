#include <zephyr/device.h>
void start_boost_detect(const struct device* imu, const struct device* altimeter);
void stop_boost_detect();