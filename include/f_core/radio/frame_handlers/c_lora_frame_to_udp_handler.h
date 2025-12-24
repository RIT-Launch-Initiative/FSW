#ifndef C_LORA_FRAME_TO_UDP_HANDLER_H
#define C_LORA_FRAME_TO_UDP_HANDLER_H

#include "f_core/net/transport/c_udp_socket.h"
#include "f_core/radio/c_lora_frame_handler.h"


class CLoraFrameToUdpHandler : public CLoraFrameHandler {
public:
    /**
     * @brief Constructor
     * @param sock Socket to use for UDP transmission
     * @param srcPort Source port for UDP packets
     */
    explicit CLoraFrameToUdpHandler(const CUdpSocket& sock, const uint16_t srcPort);


    /**
     * See parent docs
     */
    void HandleFrame(const LaunchLoraFrame& frame) override;
private:
    CUdpSocket sock;
    const uint16_t srcPort;
};


#endif //C_LORA_FRAME_TO_UDP_HANDLER_H