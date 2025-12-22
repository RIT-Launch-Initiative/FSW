#ifndef C_LORA_LINK_H
#define C_LORA_LINK_H

#include <cstddef>
#include <array>
#include <cstring>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <f_core/radio/c_lora.h>

static constexpr uint16_t RADIO_MAX_FRAME_SIZE = 256;

// Launch standardized LoRa packet.
typedef struct __attribute__((packed)) {
    uint16_t Port;
    uint8_t Size;
    uint8_t Payload[256 - sizeof(uint16_t)];
} LaunchLoraFrame;

class CLoraLink {
public:
    explicit CLoraLink(CLora& lora) : lora(lora) {}

    /**
     * @brief Send a raw payload on a given port.
     * @param[in] data Pointer to the payload data
     * @param[in] len Length of the payload data
     * @return 0 on success, negative errno on error
     */
    int Send(const uint8_t* data, uint16_t len);

    /**
     * @brief Send a LaunchLoraFrame.
     * @param[in] frame Frame to send
     * @return 0 on success, negative errno on error
     */
    int Send(const LaunchLoraFrame& frame);


    /**
     * @brief Blocking receive with timeout.
     * @param[out] frame Frame to fill with received data
     * @param[in] timeout Timeout for receiving data
     * @return >=0 length received on success, negative errno on error
     */
    int Receive(LaunchLoraFrame& frame, k_timeout_t timeout, int16_t *rssi = nullptr, int8_t *snr = nullptr);

private:
    CLora& lora;
    std::array<uint8_t, RADIO_MAX_FRAME_SIZE> rxBuffer;
};

#endif // C_LORA_LINK_H
