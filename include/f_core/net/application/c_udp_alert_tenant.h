#ifndef C_UDP_ALERT_TENANT_H
#define C_UDP_ALERT_TENANT_H

#include "f_core/n_alerts.h"
#include "f_core/net/network/c_ipv4.h"
#include "f_core/net/transport/c_udp_socket.h"
#include "f_core/os/c_callback_tenant.h"
#include "f_core/utils/c_observer.h"

#include <vector>

class CUdpAlertTenant : public CCallbackTenant {
public:
    explicit CUdpAlertTenant(const char* name, const char* ipAddrStr, const uint16_t port) : CCallbackTenant(name), sock(CUdpSocket(CIPv4(ipAddrStr), port, port)) {};

    /**
     * See parent docs
     */
    void Register() override;

    /**
     * See parent docs
     */
    void Callback() override;

    /**
     * Subscribe an observer to receive alerts
     * @param observer Observer to subscribe for alerts
     */
    void Subscribe(CObserver* observer);

    /**
     * Process a (potential) alert packet
     * @param packet Packet to process
     */
    void ProcessPacket(const NAlerts::AlertPacket & packet);
private:
    CUdpSocket sock;
    std::vector<CObserver*> observers;
};



#endif //C_UDP_ALERT_TENANT_H
