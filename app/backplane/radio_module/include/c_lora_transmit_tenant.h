#ifndef C_LORA_TRANSMIT_TENANT_H
#define C_LORA_TRANSMIT_TENANT_H

#include "n_radio_module_types.h"
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>

#include <f_core/radio/c_lora.h>
#include <f_core/c_pad_flight_landing_state_machine.h>
#include <f_core/utils/c_observer.h>
#include <f_core/utils/c_hashmap.h>

class CLoraTransmitTenant : public CTenant, public CPadFlightLandedStateMachine {
public:
    friend class CLoraReceiveTenant;

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

private:
    static constexpr uint8_t totalPortsListenedTo = 3;

    /**
     * Helper function for converting struct into a uint8_t buffer and transmitting over LoRa
     * @param[in] data Radio broadcast data structure
     * @return 0 on success, negative on error
     */
    int transmit(const NTypes::RadioBroadcastData& data) const;

    /**
     * Helper function for reading from the transmit queue
     * @param[out] data Data received from queue to transmit over LoRa
     * @return True if new data needs to be transmitted, false otherwise
     */
    bool readTransmitQueue(NTypes::RadioBroadcastData& data) const;

    CLora& lora;
    CMessagePort<NTypes::RadioBroadcastData>& loraTransmitPort;
    CHashMap<uint16_t, NTypes::RadioBroadcastData> portDataMap; // 254 bytes to leave room for port
    CHashMap<uint16_t, bool> padDataRequestedMap;
};

#endif //C_LORA_TRANSMIT_TENANT_H
