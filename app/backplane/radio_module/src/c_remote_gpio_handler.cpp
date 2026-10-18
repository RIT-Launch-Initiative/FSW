#include "c_remote_gpio_handler.h"

#include "zephyr/logging/log.h"

#include <n_autocoder_network_defs.h>

LOG_MODULE_REGISTER(CRemoteGpioHandler);

void CRemoteGpioHandler::HandleFrame(const ReceivedLaunchLoraFrame& rxFrame) {
    const LaunchLoraFrame& frame = rxFrame.Frame;
    if (frame.Size != sizeof(uint8_t)) {
        LOG_WRN("Invalid GPIO frame size: %d", frame.Size);
        return;
    }

    const uint8_t commandByte = frame.Payload[0];
    LOG_INF("Received GPIO command byte: 0x%02X", commandByte);

    setGpios(commandByte);
    transmitStatus();
}

void CRemoteGpioHandler::setGpios(const uint8_t commandByte) {
    for (size_t i = 0; i < 4; ++i) {
        bool state = (commandByte >> i) & 0b1;
        gpios[i].SetPin(state);
        LOG_INF("Set GPIO %d to %d", i, state);
    }
}

void CRemoteGpioHandler::transmitStatus() const {
    uint8_t statusByte = 0;
    for (size_t i = 0; i < 4; ++i) {
        bool state = gpios[i].GetPin();
        statusByte |= state << i;
    }

    LaunchLoraFrame statusFrame{
        .Port = NNetworkDefs::RADIO_MODULE_COMMAND_RESPONSE_PORT,
        .Size = sizeof(uint8_t),
    };
    statusFrame.Payload[0] = statusByte;
    const int ret = loraDownlinkMessagePort.Send(statusFrame, K_NO_WAIT);
    if (ret < 0) {
        LOG_ERR("Failed to send GPIO status frame (%d)", ret);
    } else {
        LOG_INF("Transmitted GPIO status byte: 0x%02X", statusByte);
    }
}