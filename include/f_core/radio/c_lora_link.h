#ifndef C_LORA_LINK_H
#define C_LORA_LINK_H

#include <cstddef>
#include <array>
#include <cstring>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <f_core/radio/c_lora.h>

static constexpr std::size_t RADIO_MAX_FRAME_SIZE = 256;

// Launch standardized LoRa packet.
typedef struct __attribute__((packed)) {
    uint16_t Port;
    uint8_t Size;
    uint8_t Payload[256 - sizeof(uint16_t)];
} LoRaBroadcastData;

struct RadioFrame {
    uint16_t port;
    uint8_t* payload;
    uint16_t length;
};

class CLoraLink {
public:
    explicit CLoraLink(CLora& lora) : lora(lora) {}

    /**
     * Send a raw payload on a logical port.
     *
     * Layout on-air:
     *   [port_lo][port_hi][payload...]
     *
     * @return >=0 length sent on success, negative errno on error.
     */

    /**
     * @brief Format and send a broadcast message over LoRa.
     * @param[in] port Port number to be the first two bytes of the message
     * @param[in] data Pointer to the payload data
     * @param[in] len Length of the payload data
     * @return 0 on success, negative errno on error
     */
    int Send(uint16_t port, const uint8_t* data, uint16_t len);


    /**
     * @brief Blocking receive with timeout.
     * @param[out] frame Frame to fill with received data
     * @param[in] timeout Timeout for receiving data
     * @return >=0 length received on success, negative errno on error
     */
    int Receive(RadioFrame& frame, k_timeout_t timeout);

private:
    CLora& lora;
    std::array<uint8_t, RADIO_MAX_FRAME_SIZE> rxBuffer;
};

#endif // C_LORA_LINK_H
