#ifndef C_LORA_FREQ_CHANGE_TENANT_H
#define C_LORA_FREQ_CHANGE_TENANT_H

#include <f_core/os/c_runnable_tenant.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/radio/c_lora.h>
#include <f_core/radio/c_lora_frame_handler.h>
#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include <zephyr/kernel.h>

#include "f_core/utils/c_soft_timer.h"

class CLoraFreqChangeTenant : public CRunnableTenant, public CLoraFrameHandler {
public:
    /**
     * @param ipStr IP to bind the UDP socket
     * @param lora Radio instance to change frequency on and to receive ACK via CLoraLink
     * @param commandUdpPort UDP port to receive frequency change commands and send to transmitters
     * @param downlinkMessagePort Message port used to queue LaunchLoraFrame for downlink
     * @param rxTimeout Timeout waiting for LoRa ACK in millis
     */
    CLoraFreqChangeTenant(const char* ipStr,
                          CLora& lora,
                          const uint16_t commandUdpPort,
                          CMessagePort<LaunchLoraFrame>& downlinkMessagePort,
                          k_timeout_t rxTimeout = K_SECONDS(15));

    /**
     * See parent docs
     */
    void HandleFrame(const LaunchLoraFrame& frame) override;

    /**
     * See parent docs
     */
    void Run() override;

    void RevertFrequency();

private:
    bool receiveCommand(float& freqMhz);
    bool sendFrequencyCommand(float freqMhz);

    CLora& lora;
    CUdpSocket udp;
    CMessagePort<LaunchLoraFrame>& downlinkMessagePort;

    float prevFreqMhz = 0.0f;
    const uint16_t commandUdpPort;
    const k_timeout_t rxTimeout;
    CSoftTimer ackTimer;
};

#endif // C_LORA_FREQ_CHANGE_TENANT_H

