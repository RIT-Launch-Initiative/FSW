#ifndef C_LORA_LINK_H
#define C_LORA_LINK_H

#include <array>

#include <zephyr/kernel.h>

#include <f_core/radio/c_lora.h>

#include "f_core/messaging/c_msgq_message_port.h"

static constexpr uint16_t RADIO_MAX_FRAME_SIZE = UINT8_MAX;

// Launch standardized LoRa packet.
typedef struct __attribute__((packed)) {
    uint16_t Port;
    uint8_t Size;
    uint8_t Payload[256 - sizeof(uint16_t)];
} LaunchLoraFrame;

typedef struct {
    uint8_t data[RADIO_MAX_FRAME_SIZE];
    uint16_t len;
    int16_t rssi;
    int8_t snr;
} ReceivedLoraRawFrame;

// Forward declare the RX callback
extern "C" void loraLinkRxCallback(const device* dev,
                                  uint8_t* data,
                                  uint16_t size,
                                  int16_t rssi,
                                  int8_t snr,
                                  void* userData);

class CLoraLink {
public:
    /**
     * @brief Constructor
     * @param lora[in] LoRa device
     */
    explicit CLoraLink(CLora& lora);

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
    int Receive(LaunchLoraFrame& frame, k_timeout_t timeout, int16_t* rssi = nullptr, int8_t* snr = nullptr);

private:
    CLora& lora;
    std::array<uint8_t, RADIO_MAX_FRAME_SIZE> rxBuffer;

    // Async Receive Queue
    static constexpr int RX_QUEUE_BUFFER_LEN = 10;
    char rxQueueBuffer[sizeof(ReceivedLoraRawFrame) * RX_QUEUE_BUFFER_LEN];
    k_msgq rxMsgq;
    CMsgqMessagePort<ReceivedLoraRawFrame> rxQueue = CMsgqMessagePort<ReceivedLoraRawFrame>(rxMsgq);

    /**
     * @brief Enqueue a received frame into the RX queue.
     * @param[in] receivedFrame Frame to enqueue
     */
    void enqueueReceivedFrame(const ReceivedLoraRawFrame& receivedFrame);


    /**
     * @brief Send a raw payload on a given port.
     * @param[in] data Pointer to the payload data
     * @param[in] len Length of the payload data
     * @return 0 on success, negative errno on error
     */
    int send(const uint8_t* data, uint16_t len);

    /**
     * Callback function for asynchronous LoRa RX
     * See Zephyr docs for parameters
     */
    friend void loraLinkRxCallback(const device* dev, uint8_t* data, uint16_t size,
                                   int16_t rssi, int8_t snr, void* userData);
};

#endif // C_LORA_LINK_H
