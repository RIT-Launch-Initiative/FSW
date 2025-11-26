#include "f_core/radio/c_lora_link.h"

#include <cstddef>
#include <array>
#include <cstring>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <f_core/radio/c_lora.h>

LOG_MODULE_REGISTER(CLoraLink);

int CLoraLink::Send(uint16_t port, const uint8_t* data, uint16_t len) {
    if (len + 2 > RADIO_MAX_FRAME_SIZE) {
        LOG_ERR("RadioLink: payload too large (%u)", len);
        return -EMSGSIZE;
    }

    std::array<uint8_t, RADIO_MAX_FRAME_SIZE> buffer{};
    buffer[0] = static_cast<uint8_t>(port & 0xFF);
    buffer[1] = static_cast<uint8_t>((port >> 8) & 0xFF);
    if (len > 0 && data != nullptr) {
        memcpy(&buffer[2], data, len);
    }

    const int ret = lora.TransmitSynchronous(buffer.data(), len + 2);
    if (ret < 0) {
        LOG_ERR("TX failed (%d)", ret);
    } else {
        LOG_DBG("TX %d bytes on port %u", ret, port);
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
            LOG_ERR("RadioLink: RX error (%d)", sz);
        }
        return size;
    }

    if (size == 0) {
        LOG_WRN("RadioLink: RX 0 bytes");
        return 0;
    }

    if (size < 2) {
        LOG_WRN("RadioLink: RX too small for header (%d)", sz);
        return -EINVAL;
    }

    const uint16_t port = static_cast<uint16_t>(rxBuffer[0]) | (static_cast<uint16_t>(rxBuffer[1]) << 8);

    frame.Port = port;
    frame.Size = static_cast<uint16_t>(size - 2);
    memcpy(frame.Payload, &rxBuffer[2], frame.Size);

    return frame.Size;
}


