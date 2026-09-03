#pragma once

#include "f_core/net/transport/c_udp_socket.h"
#include "f_core/radio/c_lora_frame_handler.h"

class CLoraFrameToUdpHandler : public CLoraFrameHandler {
  public:
    /**
     * @brief Constructor
     * @param ip IP address to bind to
     * @param srcPort Source port to bind to
     * @param statsPort Port to emit LoRa receive stats (RSSI/SNR) on; 0 disables
     */
    explicit CLoraFrameToUdpHandler(const char* ip, uint16_t srcPort, uint16_t statsPort = 0);

    /**
     * See parent docs
     */
    void HandleFrame(const ReceivedLaunchLoraFrame& rxFrame) override;

  private:
    CUdpSocket sock;
    uint16_t statsPort;
};
