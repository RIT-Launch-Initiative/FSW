#ifndef RADIO_MODULE_C_DOWNLINK_SCHEDULER_TENANT_H
#define RADIO_MODULE_C_DOWNLINK_SCHEDULER_TENANT_H

#include <f_core/radio/c_lora_frame_handler.h>
#include <f_core/os/c_runnable_tenant.h>
#include <f_core/state_machine/c_pad_flight_landing_state_machine.h>
#include <f_core/utils/c_hashmap.h>
#include <f_core/utils/c_soft_timer.h>

#include <memory>

class CDownlinkSchedulerTenant : public CRunnableTenant, public CPadFlightLandedStateMachine, public CLoraFrameHandler {
public:
    /**
     * Constructor
     * @param loraDownlinkMessagePort The message port to send downlink frames to
     * @param telemetryMessagePortMap The map of telemetry port to message port for that telemetry
     * @param telemetryDownlinkTimes The map of telemetry port to downlink interval
     */
    explicit CDownlinkSchedulerTenant(CMessagePort<LaunchLoraFrame>& loraDownlinkMessagePort,
                                      const CHashMap<uint16_t, CMessagePort<LaunchLoraFrame>*>& telemetryMessagePortMap,
                                      CHashMap<uint16_t, k_timeout_t>& telemetryDownlinkTimes);

    /**
     * See parent docs
     */
    void HandleFrame(const ReceivedLaunchLoraFrame& rxFrame) override;

    /**
     * See parent docs
     */
    void Run() override;

protected:

    /**
     * See parent docs
     */
    void PadRun() override;

    /**
     * See parent docs
     */
    void FlightRun() override;

    /**
     * See parent docs
     */
    void LandedRun() override;

    /**
     * See parent docs
     */
    void GroundRun() override;

private:
    CMessagePort<LaunchLoraFrame>& loraDownlinkMessagePort;
    CHashMap<uint16_t, CMessagePort<LaunchLoraFrame>*> telemetryMessagePortMap;
    CHashMap<uint16_t, std::unique_ptr<CSoftTimer>> telemetryDownlinkTimers;

    bool gnssDownlinkAvailable = false;
};


#endif //RADIO_MODULE_C_DOWNLINK_SCHEDULER_TENANT_H
