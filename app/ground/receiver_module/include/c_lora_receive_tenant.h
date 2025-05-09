#ifndef C_LORA_RECEIVE_TENANT_H
#define C_LORA_RECEIVE_TENANT_H

#include <c_lora_transmit_tenant.h>

#include <f_core/os/c_tenant.h>
#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/utils/c_soft_timer.h>

void shutoffTimerExpirationFn(k_timer* timer);

class CLoraReceiveTenant : public CTenant {
public:
    explicit CLoraReceiveTenant(const char* name, CLoraTransmitTenant& loraTransmitTenant, const char* ip, const uint16_t srcPort)
        : CTenant(name), loraTransmitTenant(loraTransmitTenant), udp(CUdpSocket(CIPv4(ip), srcPort, srcPort)) {}

    /**
     * See Parent Docs
     */
    ~CLoraReceiveTenant() override = default;

    /**
     * See Parent Docs
     */
    void Startup() override;

    /**
     * See Parent Docs
     */
    void PostStartup() override;

    /**
     * See Parent Docs
     */
    void Run() override;

private:
    CLoraTransmitTenant& loraTransmitTenant;

    CUdpSocket udp; 
    static constexpr int portOffset = 2;

    int receive(uint8_t *buffer, const int size, int *port);
};

#endif //C_LORA_RECEIVE_TENANT_H
