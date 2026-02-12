#include "f_core/radio/application/c_callsign_broadcast_tenant.h"


static void broadcastTimerCallbackFunction(k_timer *timer) {
    auto *tenant = static_cast<CCallsignBroadcastTenant*>(k_timer_user_data_get(timer));
    if (tenant) {
        tenant->Callback();
    }
}

CCallsignBroadcastTenant::CCallsignBroadcastTenant(const std::string& callsign, k_timeout_t transmitFrequency, CMessagePort<LaunchLoraFrame>& txPort)
    : CCallbackTenant("CCallsignBroadcastTenant"), txPort(txPort), broadcastTimer(broadcastTimerCallbackFunction), callsign(callsign) {
    broadcastTimer.SetUserData(this);
    broadcastTimer.StartTimer(transmitFrequency);
}

void CCallsignBroadcastTenant::Register() {
    // Nothing to do here
}

void CCallsignBroadcastTenant::Callback() {
    LaunchLoraFrame frame = {};
    frame.Port = 1;
    frame.Size = static_cast<uint8_t>(callsign.size());
    memcpy(frame.Payload, callsign.data(), frame.Size);
    txPort.Send(frame, K_NO_WAIT);
};
