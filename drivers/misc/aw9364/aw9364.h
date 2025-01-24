#include <zephyr/device.h>

// range 0-15
int aw9364_set_brightness(const struct device *dev, uint8_t brightness);