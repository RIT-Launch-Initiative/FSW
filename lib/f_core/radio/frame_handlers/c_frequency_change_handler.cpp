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

    LOG_INF("Changing frequency to %f Hz", static_cast<double>(newFrequencyMhz));
    int ret = lora.SetFrequency(newFrequencyMhz);

    // TODO: Do below and avoid racing :)
    // A -> B frequency change
    // A -> B ACK
    // B -> A ACK
    k_msleep(5000);

    if (ret != 0) {
        LOG_ERR("Failed to set new frequency %f Hz (%d)", static_cast<double>(newFrequencyMhz), ret);
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
    ret = loraDownlinkMessagePort.Send(ackFrame, K_NO_WAIT);
    if (ret < 0) {
        LOG_ERR("Failed to send frequency change acknowledgment (%d)", ret);
    }
}
