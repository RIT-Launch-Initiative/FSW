#include "f_core/radio/frame_handlers/c_frequency_change_handler.h"

#include "zephyr/logging/log.h"
#include <arpa/inet.h>
#include <cstring>

LOG_MODULE_REGISTER(CFrequencyChangeHandler);

void CFrequencyChangeHandler::HandleFrame(const ReceivedLaunchLoraFrame& rxFrame) {
    LaunchLoraFrame frame = rxFrame.Frame;
    if (frame.Size != sizeof(uint32_t)) {
        LOG_WRN("Frequency change frame size invalid (%d)", frame.Size);
        return;
    }

    uint32_t freqNetworkOrder = 0;
    memcpy(&freqNetworkOrder, frame.Payload, sizeof(freqNetworkOrder));
    const uint32_t freqHz = ntohl(freqNetworkOrder);
    const float freqMhz = static_cast<float>(freqHz) / 1'000'000.0f;

    LOG_INF("Changing frequency to %f MHz", static_cast<double>(freqMhz));
    const int ret = lora.SetFrequency(freqHz);

    if (ret != 0) {
        LOG_ERR("Failed to set new frequency %f MHz (%d)", static_cast<double>(freqMhz), ret);
        return;
    }

    LaunchLoraFrame ackFrame{};
    ackFrame.Port = ackPort;
    ackFrame.Size = 0; // No payload for acknowledgment

    // TODO: Going to be changed to wait for another transmit from receiver, then ACK
    k_msleep(1500);

    // Clear out the downlink message port before sending acknowledgment
    // Safe to do this since we can re-request telem if needed on pad/landing
    // And we shouldn't be changing frequency when the rocket is literally flying
    loraDownlinkMessagePort.Clear();


    const int sendRet = loraDownlinkMessagePort.Send(ackFrame, K_NO_WAIT);
    if (sendRet < 0) {
        LOG_ERR("Failed to send frequency change acknowledgment (%d)", sendRet);
    }
}
