#pragma once

#include <string>

#include "f_core/os/c_callback_tenant.h"
#include "f_core/radio/c_lora_link.h"
#include "f_core/utils/c_soft_timer.h"

class CCallsignBroadcastTenant : CCallbackTenant {
public:
    CCallsignBroadcastTenant(const std::string& callsign, k_timeout_t transmitFrequency, CMessagePort<LaunchLoraFrame>& txPort);

    void Register() override;

    void Callback() override;

private:
    CMessagePort<LaunchLoraFrame>& txPort;
    CSoftTimer broadcastTimer;
    const k_timeout_t transmitFrequency;
    const std::string_view callsign;
};
