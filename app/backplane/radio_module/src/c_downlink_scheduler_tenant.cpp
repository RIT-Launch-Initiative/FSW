#include "c_downlink_scheduler_tenant.h"

void CDownlinkSchedulerTenant::HandleFrame(const LaunchLoraFrame& frame) {
    if (this->state == State::PAD) {
        for (size_t i = 0; i + 1 < frame.Size; i += 2) {
            uint16_t port = (static_cast<uint16_t>(frame.Payload[i]) << 8) | static_cast<uint16_t>(frame.Payload[i + 1]);
            auto portMsgPortOpt = telemetryMessagePortMap.Get(port);
            if (!portMsgPortOpt.has_value()) {
                continue;
            }

            CMessagePort<LaunchLoraFrame>* portMsgPort = portMsgPortOpt.value();
            LaunchLoraFrame telemFrame{};
            int ret = portMsgPort->Receive(telemFrame, K_NO_WAIT);
            if (ret < 0) {
                LOG_ERR("Failed to receive telemetry frame on port %d", port);
            }

            ret = loraDownlinkMessagePort.Send(telemFrame, K_NO_WAIT);
            if (ret < 0) {
                LOG_ERR("Failed to send telemetry frame on port %d", port);
            }
        }
    }
}

void CDownlinkSchedulerTenant::Run() {
    this->Clock();
}

void CDownlinkSchedulerTenant::PadRun() {
    // HandleFrame handles this :)
    // Unless we want something like heartbeats or something
}
void CDownlinkSchedulerTenant::FlightRun() {
    // TODO: Implement some sort of weighting. Right now just blast
}
void CDownlinkSchedulerTenant::LandedRun() {
    // Only transmit GPS every 5 seconds
}
void CDownlinkSchedulerTenant::GroundRun() {
    return;
}
