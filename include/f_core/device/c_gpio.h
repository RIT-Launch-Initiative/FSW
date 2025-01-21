#ifndef C_GPIO_DEVICE
#define C_GPIO_DEVICE

#include <zephyr/drivers/gpio.h>

class CGpio {
public:
    /**
     * Constructor
     * @param[in] dev Zephyr device structure
     * @param[in] output bool, true if pin is for output
     */
    explicit CGpio(const gpio_dt_spec& gpioDev, const bool output = true) : gpioDev(&gpioDev) {
        if (output) {
            gpio_pin_configure(gpioDev.port, gpioDev.pin, GPIO_OUTPUT);
        } else {
            gpio_pin_configure(gpioDev.port, gpioDev.pin, GPIO_INPUT);
        }
    }

    /**
     * Gets the logical level of the pin
     * @return 1 if active, 0 if inactive, -EIO if I/O error, -EWOULDBLOCK if operation would block
     */
    int PinGet() const;

    /**
     * Sets the logical level of the pin
     * @param value[in] level to set the pin to
     * @return 0 if successful, -EIO if I/O error, -EWOULDBLOCK if operation would block
     */
    int PinSet(int value);

    /**
     * Toggles the logical level of the pin
     * @return 0 if successful, -EIO if I/O error, -EWOULDBLOCK if operation would block
     */
    int PinToggle();

    const gpio_dt_spec* GetDev();

private:
    const gpio_dt_spec* gpioDev;
};

#endif // C_GPIO_DEVICE
