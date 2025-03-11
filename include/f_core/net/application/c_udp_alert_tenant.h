#ifndef C_UDP_ALERT_TENANT_H
#define C_UDP_ALERT_TENANT_H

#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include "f_core/os/c_tenant.h"

class CUdpAlertTenant : public CTenant {
public:
    explicit CUdpAlertTenant(const char* name, const char* ipAddrStr, const uint16_t port) : CTenant(name), sock(CUdpSocket(CIPv4(ipAddrStr), port, port)) {};

    /**
     * See parent docs
     */
    void Startup() override = delete;

    /**
     * See parent docs
     */
    void PostStartup() override = delete;

    /**
     * See parent docs
     */
    void Run() override;

    /**
     * See parent docs
     */
    void Cleanup() override = delete;

private:
    CUdpSocket sock;

};



#endif //C_UDP_ALERT_TENANT_H
