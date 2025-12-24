#ifndef C_FREQUENCY_CHANGE_HANDLER_H
#define C_FREQUENCY_CHANGE_HANDLER_H

#include "f_core/radio/c_lora.h"
#include "f_core/radio/c_lora_frame_handler.h"

class CFrequencyChangeHandler : public CLoraFrameHandler {
public:
    CFrequencyChangeHandler(CLora& lora, CMessagePort<LaunchLoraFrame>& loraDownlinkMessagePort,
                            const uint16_t ackPort) :
        lora(lora), loraDownlinkMessagePort(loraDownlinkMessagePort), ackPort(ackPort) {}

    void HandleFrame(const LaunchLoraFrame& frame) override;

private:
    CLora& lora;
    CMessagePort<LaunchLoraFrame>& loraDownlinkMessagePort;
    const uint16_t ackPort;
};

#endif //C_FREQUENCY_CHANGE_HANDLER_H
