#include "f_core/radio/frame_handlers/c_lora_frame_to_udp_handler.h"

CLoraFrameToUdpHandler::CLoraFrameToUdpHandler(CUdpSocket sock, uint16_t srcPort) :
    sock(sock), srcPort(srcPort) {}

void CLoraFrameToUdpHandler::HandleFrame(const uint8_t* payload, size_t size,
                                         const char* destIp, uint16_t destPort) {
}