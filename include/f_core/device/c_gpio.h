#ifndef C_GPIO_DEVICE
#define C_GPIO_DEVICE

#include <zephyr/device.h>

class CGpio {
public:
    /**
     * Constructor
     * @param[in] dev Zephyr device structure
     */
    explicit CGpio(const device& dev) : dev(&dev) {}

    /**
     * Gets the logical level of the pin
     * @return 1 if active, 0 if inactive, -EIO if I/O error, -EWOULDBLOCK if operation would block
     */
    int pin_get();

    /**
     * Sets the logical level of the pin
     * @param value[in] level to set the pin to
     * @return 0 if successful, -EIO if I/O error, -EWOULDBLOCK if operation would block
     */
    int pin_set(int value);

private:
    const device* dev;
};

#endif // C_GPIO_DEVICE