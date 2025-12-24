#include "f_core/radio/frame_handlers/c_lora_frame_to_udp_handler.h"

CLoraFrameToUdpHandler::CLoraFrameToUdpHandler(const CUdpSocket& sock, const uint16_t srcPort) :
    sock(sock), srcPort(srcPort) {}

void CLoraFrameToUdpHandler::HandleFrame(const LaunchLoraFrame& frame) {
    sock.SetDstPort(frame.Port);
    sock.TransmitAsynchronous(&frame.Payload, frame.Size, srcPort);
}