#ifndef C_LORA_TRANSMIT_TENANT_H
#define C_LORA_TRANSMIT_TENANT_H

#include <array>

#include "n_radio_module_types.h"
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>

#include <f_core/net/device/c_lora.h>
#include <f_core/c_pad_flight_landing_state_machine.h>
#include <f_core/utils/c_hashmap.h>

class CLoraTransmitTenant : public CTenant, public PadFlightLandedStateMachine {
public:
    explicit CLoraTransmitTenant(const char* name, CLora& lora,
                                 CMessagePort<NTypes::RadioBroadcastData>* loraTransmitPort)
        : CTenant(name), lora(lora), loraTransmitPort(*loraTransmitPort) {}

    ~CLoraTransmitTenant() override = default;

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
    static constexpr uint8_t totalPortsListenedTo = 3;

    void transmit(uint8_t *data, uint8_t size) {}

    CLora& lora;
    CMessagePort<NTypes::RadioBroadcastData>& loraTransmitPort;
    CHashMap<uint16_t, std::array<uint8_t, 256>, totalPortsListenedTo> portDataMap;
};

#endif //C_LORA_TRANSMIT_TENANT_H
