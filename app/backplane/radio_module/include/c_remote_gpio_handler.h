#ifndef RADIO_MODULE_C_REMOTE_GPIO_HANDLER_H
#define RADIO_MODULE_C_REMOTE_GPIO_HANDLER_H

#include <f_core/radio/c_lora_frame_handler.h>
#include <f_core/device/c_gpio.h>

#include <zephyr/drivers/gpio.h>

class CRemoteGpioHandler : public CLoraFrameHandler {
public:
    /**
     * Handle incoming GPIO command frame
     * @param frame Frame containing GPIO command
     */
    void HandleFrame(const LaunchLoraFrame& frame) override;

private:
    CGpio gpios[4] = {
        CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(gpio0), gpios)),
        CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(gpio1), gpios)),
        CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(gpio2), gpios)),
        CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(gpio3), gpios))
    };

    /**
     * Set GPIO states based on command byte
     * @param commandByte Bitfield representing GPIO states
     */
    void setGpios(const uint8_t commandByte);

    /**
     * Transmit current GPIO states back over LoRa
     */
    void transmitStatus() const;
};


#endif //RADIO_MODULE_C_REMOTE_GPIO_HANDLER_H
