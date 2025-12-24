#include "f_core/radio/frame_handlers/c_lora_frame_to_udp_handler.h"

CLoraFrameToUdpHandler::CLoraFrameToUdpHandler(const CUdpSocket& sock) :
    sock(sock) {}

void CLoraFrameToUdpHandler::HandleFrame(const LaunchLoraFrame& frame) {
    sock.TransmitAsynchronous(&frame.Payload, frame.Size, frame.Port);
}