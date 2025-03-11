#ifndef C_UDP_ALERT_TENANT_H
#define C_UDP_ALERT_TENANT_H

#include <vector>
#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include "f_core/os/c_tenant.h"
#include "f_core/os/tenants/c_observer_tenant.h"

class CUdpAlertTenant : public CTenant {
public:
    explicit CUdpAlertTenant(const char* name, const char* ipAddrStr, const uint16_t port) : CTenant(name), sock(CUdpSocket(CIPv4(ipAddrStr), port, port)) {};

    /**
     * See parent docs
     */
    void Run() override;


    /**
     * Subscribe an observer to receive alerts
     * @param observer Observer to subscribe for alerts
     */
    void Subscribe(CObserverTenant* observer);

private:
    CUdpSocket sock;
    std::vector<CObserverTenant*> observers;
};



#endif //C_UDP_ALERT_TENANT_H
