#ifndef C_UDP_ALERT_TENANT_H
#define C_UDP_ALERT_TENANT_H

#include <array>
#include <vector>
#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include "f_core/os/c_tenant.h"
#include "f_core/os/tenants/c_observer_tenant.h"

class CUdpAlertTenant : public CTenant {
public:

    // TODO: Autocoder?
    // 16 bits instead of 8 for potential future expansion (Although I hope we don't have to deal with >255 events!)
    typedef enum : uint16_t {
        BOOST = 'b',
        APOGEE = 'a',
        NOSEOVER = 'n',
        LANDED = 'l',
    } AlertType;

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

    /**
     * Subscribe an observer to receive alerts
     * @param observer Observer to subscribe for alerts
     */
    void Subscribe(CObserverTenant& observer);

private:
    // Magic byte signature, to limit the chance of randomness misfiring an alert
    static constexpr std::array<uint8_t, 6> MAGIC_BYTE_SIGNATURE = {'L', 'A', 'U', 'N', 'C', 'H'};
    static constexpr size_t MAGIC_BYTE_SIGNATURE_SIZE = sizeof(MAGIC_BYTE_SIGNATURE);
    static constexpr size_t ALERT_PACKET_SIZE = MAGIC_BYTE_SIGNATURE_SIZE + sizeof(AlertType);

    CUdpSocket sock;
    std::vector<CObserverTenant*> observers;
};



#endif //C_UDP_ALERT_TENANT_H
