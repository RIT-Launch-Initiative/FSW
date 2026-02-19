#include "f_core/radio/application/c_callsign_broadcast_tenant.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CCallsignBroadcastTenant);

static void broadcastTimerCallbackFunction(k_timer *timer) {
    auto *tenant = static_cast<CCallsignBroadcastTenant*>(k_timer_user_data_get(timer));
    if (tenant) {
        tenant->Callback();
    }
}

CCallsignBroadcastTenant::CCallsignBroadcastTenant(const char* callsign, k_timeout_t transmitFrequency, CMessagePort<LaunchLoraFrame>& txPort, const uint8_t loraBroadcastPort)
    : CCallbackTenant("CCallsignBroadcastTenant"), txPort(txPort),
      broadcastTimer(broadcastTimerCallbackFunction), transmitFrequency(transmitFrequency), callsign(callsign), loraBroadcastPort(loraBroadcastPort) {
}

void CCallsignBroadcastTenant::Register() {
    broadcastTimer.SetUserData(this);
    broadcastTimer.StartTimer(transmitFrequency);
}

void CCallsignBroadcastTenant::Callback() {
    LaunchLoraFrame frame = {};
    frame.Port = loraBroadcastPort;
    frame.Size = static_cast<uint8_t>(callsign.size());
    if (frame.Size > sizeof(frame.Payload)) {
        LOG_ERR("Callsign size %zu exceeds maximum payload size %zu", callsign.size(), sizeof(frame.Payload));
        return;
    }

    memcpy(frame.Payload, callsign.data(), frame.Size);
    txPort.Send(frame, K_NO_WAIT);
};
