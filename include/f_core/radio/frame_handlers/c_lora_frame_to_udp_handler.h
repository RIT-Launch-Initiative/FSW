#ifndef C_LORA_FRAME_TO_UDP_HANDLER_H
#define C_LORA_FRAME_TO_UDP_HANDLER_H

#include "f_core/net/transport/c_udp_socket.h"
#include "f_core/radio/c_lora_frame_handler.h"


class CLoraFrameToUdpHandler : public CLoraFrameHandler {
public:
    /**
     * @brief Constructor
     * @param ip IP address instance to bind to
     * @param srcPort Source port to bind to
     */
    explicit CLoraFrameToUdpHandler(const char* ip, uint16_t srcPort);

    /**
     * See parent docs
     */
    void HandleFrame(const LaunchLoraFrame& frame) override;
private:
    CUdpSocket sock;
};


#endif //C_LORA_FRAME_TO_UDP_HANDLER_H