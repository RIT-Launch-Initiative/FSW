#ifndef C_LORA_FRAME_TO_UDP_HANDLER_H
#define C_LORA_FRAME_TO_UDP_HANDLER_H

#include <stdint.h>


class CLoraFrameToUdpHandler : public CLoraFrameHandler {
public:
    explicit CLoraFrameToUdpHandler(CUdpSocket sock, uint16_t srcPort);

    void HandleFrame(const uint8_t* payload, size_t size, const char* destIp, uint16_t destPort);

private:
    CUdpSocket sock;
    const uint16_t srcPort;

};


#endif //C_LORA_FRAME_TO_UDP_HANDLER_H