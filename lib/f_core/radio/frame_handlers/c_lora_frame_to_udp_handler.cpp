#include "f_core/radio/frame_handlers/c_lora_frame_to_udp_handler.h"

#include "f_core/net/network/c_ipv4.h"

CLoraFrameToUdpHandler::CLoraFrameToUdpHandler(const char *ip, uint16_t srcPort) :
    sock(CIPv4(ip), srcPort, 0) {}

void CLoraFrameToUdpHandler::HandleFrame(const ReceivedLaunchLoraFrame& rxFrame) {
    const LaunchLoraFrame& frame = rxFrame.Frame;
    sock.TransmitAsynchronous(&frame.Payload, frame.Size, frame.Port);
}