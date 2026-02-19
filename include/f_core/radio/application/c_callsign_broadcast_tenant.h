#pragma once

#include <string>

#include "f_core/os/c_callback_tenant.h"
#include "f_core/radio/c_lora_link.h"
#include "f_core/utils/c_soft_timer.h"

class CCallsignBroadcastTenant : public CCallbackTenant {
public:
    /**
     *
     * @param callsign Callsign to broadcast
     * @param transmitFrequency How often to broadcast the callsign
     * @param txPort Message port to send the callsign frame to for transmission over LoRa
     */
    CCallsignBroadcastTenant(const char* callsign, k_timeout_t transmitFrequency, CMessagePort<LaunchLoraFrame>& txPort);

    /**
     * See parent docs
     */
    void Register() override;


    /**
     * See parent docs
     */
    void Callback() override;

private:
    CMessagePort<LaunchLoraFrame>& txPort;
    CSoftTimer broadcastTimer;
    const k_timeout_t transmitFrequency;
    const std::string callsign;
    const uint8_t loraBroadcastPort;
};
