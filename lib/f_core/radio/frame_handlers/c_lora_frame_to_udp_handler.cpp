#include "f_core/radio/frame_handlers/c_lora_frame_to_udp_handler.h"

#include "f_core/net/network/c_ipv4.h"

CLoraFrameToUdpHandler::CLoraFrameToUdpHandler(const char* ip, uint16_t srcPort, uint16_t statsPort)
    : sock(CIPv4(ip), srcPort, 0), statsPort(statsPort) {}

void CLoraFrameToUdpHandler::HandleFrame(const ReceivedLaunchLoraFrame& rxFrame) {
    const LaunchLoraFrame& frame = rxFrame.Frame;
    sock.TransmitAsynchronous(&frame.Payload, frame.Size, frame.Port);

    if (statsPort != 0) {
        struct __attribute__((packed)) {
            int16_t ReceivedSignalStrength;
            int8_t SignalToNoise;
        } stats{rxFrame.ReceivedSignalStrength, rxFrame.SignalToNoise};
        sock.TransmitAsynchronous(&stats, sizeof(stats), statsPort);
    }
}