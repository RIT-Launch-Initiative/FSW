#ifndef C_GPIO_DEVICE
#define C_GPIO_DEVICE

#include <zephyr/device.h>

class CGpio {
public:
    /**
     * Constructor
     * @param[in] dev Zephyr device structure
     */
    explicit CGpio(const device& dev);

    int pin_get();

    int pin_set(int value);

private:
    const device* dev;
};

#endif // C_GPIO_DEVICE