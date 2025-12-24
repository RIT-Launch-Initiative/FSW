#include "f_core/radio/frame_handlers/c_frequency_change_handler.h"

#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(CFrequencyChangeHandler);

void CFrequencyChangeHandler::HandleFrame(const LaunchLoraFrame& frame) {
    if (frame.Size != sizeof(uint32_t)) {
        LOG_WRN("Frequency change frame size invalid (%d)", frame.Size);
        return;
    }

    uint32_t newFrequency = 0;
    for (size_t i = 0; i < sizeof(uint32_t); ++i) {
        newFrequency |= static_cast<uint32_t>(frame.Payload[i]) << (8 * i);
    }

    LOG_INF("Changing frequency to %u Hz", newFrequency);
    if (!lora.SetFrequency(newFrequency)) {
        LOG_ERR("Failed to set new frequency %u Hz", newFrequency);
        return;
    }

    // Acknowledge frequency change
    LaunchLoraFrame ackFrame{};
    ackFrame.Port = frame.Port;
    ackFrame.Size = 0; // No payload for acknowledgment
    int ret = loraDownlinkMessagePort.Send(ackFrame, K_NO_WAIT);
    if (ret < 0) {
        LOG_ERR("Failed to send frequency change acknowledgment (%d)", ret);
    }
}
