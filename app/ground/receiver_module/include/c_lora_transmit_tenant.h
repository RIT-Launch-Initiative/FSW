#ifndef C_LORA_TRANSMIT_TENANT_H
#define C_LORA_TRANSMIT_TENANT_H

#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>

#include <f_core/radio/c_lora.h>
#include <f_core/utils/c_hashmap.h>

#include <n_autocoder_types.h>

class CLoraTransmitTenant : public CTenant {
public:
    friend class CLoraReceiveTenant;

    explicit CLoraTransmitTenant(const char* name, CLora& lora,
                                 CMessagePort<NTypes::LoRaBroadcastData>* loraTransmitPort)
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

private:
    static constexpr uint8_t totalPortsListenedTo = 3;

    /**
     * Helper function for converting struct into a uint8_t buffer and transmitting over LoRa
     * @param[in] data Radio broadcast data structure
     */
    void transmit(const NTypes::LoRaBroadcastData& data) const;

    /**
     * Helper function for reading from the transmit queue
     * @param[out] data Data received from queue to transmit over LoRa
     * @return True if new data needs to be transmitted, false otherwise
     */
    bool readTransmitQueue(NTypes::LoRaBroadcastData& data) const;

    CLora& lora;
    CMessagePort<NTypes::LoRaBroadcastData>& loraTransmitPort;
    CHashMap<uint16_t, NTypes::LoRaBroadcastData> portDataMap; // 254 bytes to leave room for port
    CHashMap<uint16_t, bool> padDataRequestedMap;
};

#endif //C_LORA_TRANSMIT_TENANT_H
