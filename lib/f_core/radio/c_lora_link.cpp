#include "f_core/radio/c_lora_link.h"

#include <cstddef>
#include <array>
#include <cstring>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <f_core/radio/c_lora.h>

LOG_MODULE_REGISTER(CLoraLink);

extern "C" void loraLinkRxCallback(const device* dev, uint8_t* data, uint16_t size,
                                   int16_t rssi, int8_t snr, void* userData) {
    if (userData == nullptr) {
        LOG_ERR("loraLinkRxCallback called with null userData");
        return;
    }

    CLoraLink* loraLink = static_cast<CLoraLink*>(userData);

    if (size == 0) {
        LOG_WRN("RX 0 bytes");
        return;
    }

    if (size > RADIO_MAX_FRAME_SIZE) {
        LOG_WRN("RX too large (%u)", size);
        return;
    }

    ReceivedLoraRawFrame msg{};
    msg.Size = size;
    msg.ReceivedSignalStrength = rssi;
    msg.SignalToNoise = snr;
    memcpy(msg.Payload, data, size);

    loraLink->enqueueReceivedFrame(msg);
}

CLoraLink::CLoraLink(CLora& lora) : lora(lora) {
    k_msgq_init(&rxMsgq, rxQueueBuffer, sizeof(ReceivedLoraRawFrame), RX_QUEUE_BUFFER_LEN);
    lora.EnableAsynchronous(loraLinkRxCallback, this);
}

int CLoraLink::Send(const LaunchLoraFrame& frame) {
    std::array<uint8_t, RADIO_MAX_FRAME_SIZE> buffer{};
    buffer.at(0) = static_cast<uint8_t>(frame.Port & 0xFF);
    buffer.at(1) = static_cast<uint8_t>((frame.Port >> 8) & 0xFF);

    if (frame.Size > RADIO_MAX_FRAME_SIZE - 2) {
        return -EMSGSIZE;
    }

    if (frame.Size > 0) {
        memcpy(&buffer.at(2), frame.Payload, frame.Size);
    }

    const int ret = send(buffer.data(), frame.Size + 2);
    if (ret == 0) {
        LOG_DBG("Successfully sent port %d size %d", frame.Port, frame.Size);
    }

    return ret;
}


int CLoraLink::send(const uint8_t* data, uint16_t len) {
    lora.DisableAsynchronous();


    if (len > RADIO_MAX_FRAME_SIZE) {
        LOG_ERR("Payload too large (%u)", len);
        return -EMSGSIZE;
    }

    const int ret = lora.TransmitSynchronous(data, len);
    if (ret < 0) {
        LOG_ERR("TX of size %d failed (%d)", len, ret);
    }

    lora.EnableAsynchronous(loraLinkRxCallback, this);
    return ret;
}


int CLoraLink::Receive(ReceivedLaunchLoraFrame& frame, k_timeout_t timeout) {
    ReceivedLoraRawFrame raw{};

    const int ret = rxQueue.Receive(raw, timeout);
    if (ret < 0) {
        return ret;
    }

    if (raw.Size < 2) {
        LOG_WRN("RX too small for header (%u)", raw.len);
        return -EINVAL;
    }

    const uint16_t port = static_cast<uint16_t>(raw.Payload[0]) | (static_cast<uint16_t>(raw.Payload[1]) << 8);
    const uint16_t payloadLen = static_cast<uint16_t>(raw.Size - 2);

    if (payloadLen > sizeof(frame.Frame.Payload)) {
        LOG_WRN("RX payload too large (%u)", payloadLen);
        return -EMSGSIZE;
    }

    if (payloadLen > RADIO_MAX_FRAME_SIZE) {
        LOG_WRN("RX payload too large for Size field (%u)", payloadLen);
        return -EMSGSIZE;
    }

    frame.Frame.Port = port;
    frame.Frame.Size = static_cast<uint8_t>(payloadLen);
    if (payloadLen > 0) {
        memcpy(frame.Frame.Payload, &raw.Payload[2], payloadLen);
    }

    frame.ReceivedSignalStrength = raw.ReceivedSignalStrength;
    frame.SignalToNoise = raw.SignalToNoise;

    return payloadLen;
}

void CLoraLink::enqueueReceivedFrame(const ReceivedLoraRawFrame& receivedFrame) {
    const int ret = rxQueue.Send(receivedFrame, K_NO_WAIT);
    if (ret < 0) {
        LOG_WRN("RX queue full, dropping packet");
    }
    (void)ret;
}
