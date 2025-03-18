#ifndef C_SNTP_SERVER_H
#define C_SNTP_SERVER_H

#include "f_core/os/c_tenant.h"
#include "f_core/net/network/c_ipv4.h"
#include "f_core/net/transport/c_udp_socket.h"

class CSntpServerTenant : public CTenant {
public:
    static constexpr uint16_t SNTP_DEFAULT_PORT = 123;
    inline static CSntpServerTenant *instance = nullptr;

    /**
     * Singleton getter to avoid multiple instances of the SNTP server.
     */
    static CSntpServerTenant *getInstance(const CIPv4 &ipv4, uint16_t port = SNTP_DEFAULT_PORT) {
        if (instance == nullptr) {
            instance = new CSntpServerTenant(ipv4, port);
        }
        return instance;
    }

    /**
     * See parent docs
     */
    void Startup() override;

    /**
     * See parent docs
     */
    void Cleanup() override;

    /**
     * See parent docs
     */
    void Run() override;

private:
    CUdpSocket sock; // The socket bound to port 123 (or specified port)
    CIPv4 ip;

    CSntpServerTenant(const CIPv4 &ipv4, uint16_t port = SNTP_DEFAULT_PORT)
        : CTenant("SNTP server"), sock(ipv4, port, port), ip(ipv4) {};

};

#endif // C_SNTP_SERVER_H