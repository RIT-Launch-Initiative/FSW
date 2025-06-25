#include "f_core/net/application/c_udp_alert_tenant.h"

#include "zephyr/net/socket_service.h"

#include <f_core/n_alerts.h>

#include <array>

LOG_MODULE_REGISTER(CUdpAlertTenant);

extern net_socket_service_desc alertSocketService;

extern "C" void alertSocketServiceHandler(net_socket_service_event* pev) {
    NAlerts::AlertPacket buff{};
    auto userData = static_cast<CUdpSocket::SocketServiceUserData*>(pev->user_data);

    if (userData == nullptr) {
        LOG_ERR("User data is null in alertSocketServiceHandler");
        k_oops();
    }

    CUdpAlertTenant* tenant = static_cast<CUdpAlertTenant*>(userData->userData);
    if (tenant == nullptr) {
        LOG_ERR("Tenant is null in alertSocketServiceHandler");
        k_oops();
    }

    userData->socket->ReceiveSynchronous(buff.data(), NAlerts::ALERT_PACKET_SIZE);
    tenant->ProcessPacket(buff);
}

void CUdpAlertTenant::Startup() {
    int ret = sock.RegisterSocketService(&alertSocketService, this);
    if (ret < 0) {
        LOG_ERR("Failed to register socket service for CUdpAlertTenant: %d", ret);
    }
}

void CUdpAlertTenant::Subscribe(CObserver* observer) {
    observers.push_back(observer);
    LOG_INF("Subscribed observer %p to CUdpAlertTenant", observer);
}

void CUdpAlertTenant::Run() {
    NAlerts::AlertPacket buff{};
    if (sock.ReceiveAsynchronous(buff.data(), NAlerts::ALERT_PACKET_SIZE) > 0) {
        ProcessPacket(buff);
    }
}

void CUdpAlertTenant::ProcessPacket(const NAlerts::AlertPacket& packet) {
    for (size_t i = 0; i < NAlerts::MAGIC_BYTE_SIGNATURE_SIZE; i++) {
        if (packet[i] != NAlerts::MAGIC_BYTE_SIGNATURE[i]) {
            return;
        }
    }

    // Potential alternative is using signals, but that might require passing in a lot of variables around
    NAlerts::AlertType alertType = static_cast<NAlerts::AlertType>(packet[NAlerts::MAGIC_BYTE_SIGNATURE_SIZE]);
    LOG_INF("Received alert type %c", alertType); // Would have to be changed to %d if alerts go past 8 bits
    for (auto observer : observers) {
        LOG_INF("Notifying observer");
        observer->Notify(&alertType);
    }
}
