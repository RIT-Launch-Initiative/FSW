#ifndef C_LORA_TENANT_H
#define C_LORA_TENANT_H

#include <zephyr/kernel.h>

#include <f_core/os/c_runnable_tenant.h>
#include <f_core/state_machine/c_pad_flight_landing_state_machine.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/radio/c_lora_link.h>
#include <f_core/radio/c_lora_router.h>

class CLoraTenant : public CRunnableTenant, public CPadFlightLandedStateMachine
{
public:
    /**
     * @brief Constructor
     * @param radioLink  Protocol/framing layer over CLora.
     * @param txPort     Message port for outgoing LoRaBroadcastData.
     */
    CLoraTenant(CLoraLink& radioLink,
                     CMessagePort<LaunchLoraFrame>& txPort);

    void Startup() override;

    void PadRun() override;
    void FlightRun() override;
    void LandedRun() override;

private:
    void serviceTx();
    void serviceRx(const k_timeout_t timeout);

    void CacheDownlink(const LaunchLoraFrame& data);

    CLoraLink&  link;
    CLoraRouter router;

    CMessagePort<LaunchLoraFrame>& loraTransmitPort;
    CHashMap<uint16_t, LaunchLoraFrame> downlinkCache_;
};

#endif//C_LORA_TENANT_H