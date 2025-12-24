#ifndef RADIO_MODULE_C_DOWNLINK_SCHEDULER_TENANT_H
#define RADIO_MODULE_C_DOWNLINK_SCHEDULER_TENANT_H

#include <f_core/radio/c_lora_frame_handler.h>
#include <f_core/os/c_runnable_tenant.h>
#include <f_core/state_machine/c_pad_flight_landing_state_machine.h>

#include "f_core/utils/c_hashmap.h"
#include "f_core/utils/c_soft_timer.h"

class CDownlinkSchedulerTenant : public CRunnableTenant, public CPadFlightLandedStateMachine, public CLoraFrameHandler {
public:
    explicit CDownlinkSchedulerTenant(CMessagePort<LaunchLoraFrame>& loraDownlinkMessagePort,
                                      const CHashMap<uint16_t, CMessagePort<LaunchLoraFrame>*>& telemetryMessagePortMap,
                                      CHashMap<uint16_t, k_timeout_t>& telemetryDownlinkTimes);

    void HandleFrame(const LaunchLoraFrame& frame) override;

    void Run() override;

protected:
    void PadRun() override;
    void FlightEntry() override;
    void FlightRun() override;
    void LandedRun() override;
    void GroundRun() override;

private:
    CMessagePort<LaunchLoraFrame>& loraDownlinkMessagePort;
    CHashMap<uint16_t, CMessagePort<LaunchLoraFrame>*> telemetryMessagePortMap;
    CHashMap<uint16_t, k_timeout_t> downlinkRateMap;
    CHashMap<uint16_t, CSoftTimer> telemetryDownlinkTimers;

    bool gnssDownlinkAvailable = false;
};


#endif //RADIO_MODULE_C_DOWNLINK_SCHEDULER_TENANT_H
