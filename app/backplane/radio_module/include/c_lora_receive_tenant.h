#ifndef C_LORA_RECEIVE_TENANT_H
#define C_LORA_RECEIVE_TENANT_H


#include "n_radio_module_types.h"

#include <f_core/c_pad_flight_landing_state_machine.h>
#include <f_core/os/c_tenant.h>
#include <f_core/net/device/c_lora.h>
#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/device/c_gpio.h>

class CLoraReceiveTenant : public CTenant, public PadFlightLandedStateMachine {
public:
    explicit CLoraReceiveTenant(const char* name, CLora& lora, const char* ip, const uint16_t srcPort, CMessagePort<NTypes::RadioBroadcastData>* loraTransmitPort)
        : CTenant(name), lora(lora), udp(CUdpSocket(CIPv4(ip), srcPort, srcPort)), loraTransmitPort(*loraTransmitPort) {}

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

    /**
     * See Parent Docs
     */
    void PadRun() override;

    /**
     * See Parent Docs
     */
    void FlightRun() override;

    /**
     * See Parent Docs
     */
    void LandedRun() override;

    /**
     * See Parent Docs
     */
    void GroundRun() override;

private:
    CLora& lora;
    CUdpSocket udp; 
    CGpio gpios[4] = {
        CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(gpio0), gpios)),
        CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(gpio1), gpios)),
        CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(gpio2), gpios)),
        CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(gpio3), gpios))
    };
    CMessagePort<NTypes::RadioBroadcastData>& loraTransmitPort;
    static constexpr int portOffset = 2;

    int receive(const uint8_t *buffer, const int size, int *port);
};

#endif //C_LORA_RECEIVE_TENANT_H
