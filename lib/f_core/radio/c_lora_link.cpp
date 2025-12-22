#include "f_core/radio/c_lora_link.h"

#include <cstddef>
#include <array>
#include <cstring>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <f_core/radio/c_lora.h>

LOG_MODULE_REGISTER(CLoraLink);

int CLoraLink::Send(const LaunchLoraFrame& frame) {
    std::array<uint8_t, RADIO_MAX_FRAME_SIZE> buffer{};
    buffer.at(0) = static_cast<uint8_t>(frame.Port & 0xFF);
    buffer.at(1) = static_cast<uint8_t>((frame.Port >> 8) & 0xFF);
    if (frame.Size > 0) {
        memcpy(&buffer.at(2), frame.Payload, frame.Size);
    }

    const int ret = Send(buffer.data(), frame.Size + 2);
    if (ret == 0) {
        LOG_DBG("Successfully sent port %d size %d", frame.Port, frame.Size);
    }

    return ret;
}


int CLoraLink::Send(const uint8_t* data, uint16_t len) {
    if (len + 2 > RADIO_MAX_FRAME_SIZE) {
        LOG_ERR("Payload too large (%u)", len);
        return -EMSGSIZE;
    }

    const int ret = lora.TransmitSynchronous(data, len);
    if (ret < 0) {
        LOG_ERR("TX of size %d failed (%d)", len, ret);
    }

    return ret;
}


int CLoraLink::Receive(LaunchLoraFrame& frame, k_timeout_t timeout, int16_t *rssi, int8_t *snr) {
    const int size = lora.ReceiveSynchronous(
        rxBuffer.data(), rxBuffer.size(),
        rssi, snr,
        timeout
    );

    if (size < 0) {
        if (size != -EAGAIN) {
            LOG_ERR("RX error (%d)", size);
        }
        return size;
    }

    if (size == 0) {
        LOG_WRN("RX 0 bytes");
        return 0;
    }

    if (size < 2) {
        LOG_WRN("RX too small for header (%d)", size);
        return -EINVAL;
    }

    const uint16_t port = static_cast<uint16_t>(rxBuffer[0]) | (static_cast<uint16_t>(rxBuffer[1]) << 8);

    frame.Port = port;
    frame.Size = static_cast<uint16_t>(size - 2);
    memcpy(frame.Payload, &rxBuffer[2], frame.Size);

    return frame.Size;
}


