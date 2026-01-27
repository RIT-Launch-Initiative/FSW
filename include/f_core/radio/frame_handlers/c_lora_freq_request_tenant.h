#pragma once

#include <f_core/os/c_runnable_tenant.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/radio/c_lora.h>
#include <f_core/radio/c_lora_frame_handler.h>
#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include <zephyr/kernel.h>

#include "f_core/utils/c_soft_timer.h"

class CLoraFreqRequestTenant : public CRunnableTenant, public CLoraFrameHandler {
public:
    /**
     * @param ipStr IP to bind the UDP socket
     * @param lora Radio instance to change frequency on and to receive ACK via CLoraLink
     * @param commandUdpPort UDP port to receive frequency change commands and send to transmitters
     * @param downlinkMessagePort Message port used to queue LaunchLoraFrame for downlink
     * @param rxTimeout Timeout waiting for LoRa ACK in millis
     */
    CLoraFreqRequestTenant(const char* ipStr,
                          CLora& lora,
                          const uint16_t commandUdpPort,
                          CMessagePort<LaunchLoraFrame>& downlinkMessagePort,
                          k_timeout_t rxTimeout = K_SECONDS(15));

    /**
     * See parent docs
     */
    void HandleFrame(const ReceivedLaunchLoraFrame& rxFrame) override;

    /**
     * See parent docs
     */
    void Run() override;

    void RequestRevertFrequency();

private:
    /**
     * Receive a frequency change command over UDP
     * @param freqHz Reference to store received frequency in Hz
     * @return True if a command was received, false otherwise
     */
    bool receiveCommand(uint32_t& freqHz);

    /**
     * Transmit frequency change command over LoRa
     * @param freqHz Frequency in Hz to send command for
     * @return True if command was sent successfully, false otherwise
     */
    bool sendFrequencyCommand(uint32_t freqHz);

    void revertFrequency();



    CLora& lora;
    CUdpSocket udp;
    CMessagePort<LaunchLoraFrame>& downlinkMessagePort;

    float prevFreqMhz = 0.0f;
    uint32_t prevFreqHz = 0;
    uint32_t freqHzRequested = 0;

    const uint16_t commandUdpPort;
    const k_timeout_t rxTimeout;
    CSoftTimer ackTimer;
    bool revertFrequencyRequested = false;

};

#endif // C_LORA_FREQ_REQUEST_TENANT_H

