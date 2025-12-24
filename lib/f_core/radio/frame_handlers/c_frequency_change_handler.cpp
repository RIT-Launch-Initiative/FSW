#include "f_core/radio/frame_handlers/c_frequency_change_handler.h"

#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(CFrequencyChangeHandler);

void CFrequencyChangeHandler::HandleFrame(const LaunchLoraFrame& frame) {
    if (frame.Size != sizeof(float)) {
        LOG_WRN("Frequency change frame size invalid (%d)", frame.Size);
        return;
    }

    float newFrequencyMhz = 0.0f;
    memcpy(&newFrequencyMhz, frame.Payload, sizeof(float));

    LOG_INF("Changing frequency to %f Hz", newFrequencyMhz);
    if (!lora.SetFrequency(newFrequencyMhz)) {
        LOG_ERR("Failed to set new frequency %f Hz", newFrequencyMhz);
        return;
    }

    // Acknowledge frequency change
    LaunchLoraFrame ackFrame{};
    ackFrame.Port = frame.Port;
    ackFrame.Size = 0; // No payload for acknowledgment

    // Clear out the downlink message port before sending acknowledgment
    // Safe to do this since we can re-request telem if needed on pad/landing
    // And we shouldn't be changing frequency when the rocket is literally flying
    loraDownlinkMessagePort.Clear();

    // TODO: Should probably make the frequency persistent though...

    int ret = loraDownlinkMessagePort.Send(ackFrame, K_NO_WAIT);
    if (ret < 0) {
        LOG_ERR("Failed to send frequency change acknowledgment (%d)", ret);
    }
}
