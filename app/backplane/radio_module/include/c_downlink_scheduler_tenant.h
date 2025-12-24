#ifndef RADIO_MODULE_C_DOWNLINK_SCHEDULER_TENANT_H
#define RADIO_MODULE_C_DOWNLINK_SCHEDULER_TENANT_H

#include <f_core/radio/c_lora_frame_handler.h>
#include <f_core/os/c_runnable_tenant.h>
#include <f_core/state_machine/c_pad_flight_landing_state_machine.h>

#include "f_core/utils/c_hashmap.h"

class CDownlinkSchedulerTenant : public CRunnableTenant, public CPadFlightLandedStateMachine, public CLoraFrameHandler {
public:
    explicit CDownlinkSchedulerTenant(CMessagePort<LaunchLoraFrame>& loraDownlinkMessagePort,
                                      CHashMap<uint16_t, CMessagePort<LaunchLoraFrame>*> telemetryMessagePortMap) :
        CRunnableTenant("Downlink Scheduler"), loraDownlinkMessagePort(loraDownlinkMessagePort),
        telemetryMessagePortMap(telemetryMessagePortMap) {}

    void HandleFrame(const LaunchLoraFrame& frame) override;

    void Run() override;

protected:
    CMessagePort<LaunchLoraFrame>& loraDownlinkMessagePort;
    CHashMap<uint16_t, CMessagePort<LaunchLoraFrame>*> telemetryMessagePortMap;
    void PadRun() override;
    void FlightRun() override;
    void LandedRun() override;
    void GroundRun() override;

private:
};


#endif //RADIO_MODULE_C_DOWNLINK_SCHEDULER_TENANT_H
