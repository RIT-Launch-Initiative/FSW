#include "f_core/net/application/c_udp_alert_tenant.h"
#include <f_core/n_alerts.h>

#include <array>

LOG_MODULE_REGISTER(CUdpAlertTenant);

void CUdpAlertTenant::Subscribe(CObserver* observer) {
    observers.push_back(observer);
}

void CUdpAlertTenant::Run() {
    NAlerts::AlertPacket buff{};
    if (sock.ReceiveAsynchronous(buff.data(), NAlerts::ALERT_PACKET_SIZE) > 0) {
        for (size_t i = 0; i < NAlerts::MAGIC_BYTE_SIGNATURE_SIZE; i++) {
            if (buff[i] != NAlerts::MAGIC_BYTE_SIGNATURE[i]) {
                return;
            }
        }

        // Potential alternative is using signals, but that might require passing in a lot of variables around
        NAlerts::AlertType alertType = static_cast<NAlerts::AlertType>(buff[NAlerts::MAGIC_BYTE_SIGNATURE_SIZE]);
        LOG_INF("Received alert type %c", alertType); // Would have to be changed to %d if alerts go past 8 bits
        for (auto observer : observers) {
            LOG_INF("NOtifying observer");
            observer->Notify(&alertType);
        }
    }
}
