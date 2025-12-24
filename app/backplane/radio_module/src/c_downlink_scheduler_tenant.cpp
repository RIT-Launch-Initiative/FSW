#include "c_downlink_scheduler_tenant.h"

#include <n_autocoder_network_defs.h>

LOG_MODULE_REGISTER(CDownlinkSchedulerTenant);

CDownlinkSchedulerTenant::CDownlinkSchedulerTenant(CMessagePort<LaunchLoraFrame>& loraDownlinkMessagePort,
                                                   const CHashMap<uint16_t, CMessagePort<LaunchLoraFrame>*>&
                                                   telemetryMessagePortMap,
                                                   CHashMap<uint16_t, k_timeout_t>& telemetryDownlinkTimes) :
    CRunnableTenant("Downlink Scheduler"), loraDownlinkMessagePort(loraDownlinkMessagePort),
    telemetryMessagePortMap(telemetryMessagePortMap) {

    // Fun C++ fuckery
    for (const auto& [port, timeout] : telemetryDownlinkTimes) {
        telemetryDownlinkTimers.Emplace(port, std::make_unique<CSoftTimer>());
        telemetryDownlinkTimers.GetPtr(port)->get()->StartTimer(timeout);
    }

    gnssDownlinkAvailable = telemetryMessagePortMap.Get(NNetworkDefs::RADIO_MODULE_GNSS_DATA_PORT).has_value() &&
    ([](auto* p) { return p && p->get(); }(
        telemetryDownlinkTimers.GetPtr(NNetworkDefs::RADIO_MODULE_GNSS_DATA_PORT)));
}

void CDownlinkSchedulerTenant::HandleFrame(const LaunchLoraFrame& frame) {
    if (this->state == State::PAD || this->state == State::LANDED) {
        for (size_t i = 0; i + 1 < frame.Size; i += 2) {
            uint16_t upper = frame.Payload[i] << 8;
            uint16_t lower = frame.Payload[i + 1];
            uint16_t port = upper | lower;

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
    LOG_INF("Running");
    SetBoostDetected(NStateMachineGlobals::boostDetected);
    SetLandingDetected(NStateMachineGlobals::landingDetected);
    this->Clock();
}

void CDownlinkSchedulerTenant::PadRun() {
    // HandleFrame handles this :)
    // Unless we want something like heartbeats or something
}


void CDownlinkSchedulerTenant::FlightRun() {
    // If we are nearing full, clear out the queue to make space for the most recent data
    if (loraDownlinkMessagePort.AvailableSpace() <= telemetryMessagePortMap.Size()) {
        loraDownlinkMessagePort.Clear();
    }

    for (auto const& [port, timer] : telemetryDownlinkTimers) {
        if (timer->IsExpired()) {
            auto portMsgPortOpt = telemetryMessagePortMap.Get(port);
            if (!portMsgPortOpt.has_value()) {
                continue;
            }

            CMessagePort<LaunchLoraFrame>* portMsgPort = portMsgPortOpt.value();
            LaunchLoraFrame telemFrame{};
            int ret = portMsgPort->Receive(telemFrame, K_NO_WAIT);
            if (ret < 0) {
                LOG_ERR("Failed to receive telemetry frame on port %d", port);
                continue;
            }

            ret = loraDownlinkMessagePort.Send(telemFrame, K_NO_WAIT);
            if (ret < 0) {
                LOG_ERR("Failed to send telemetry frame on port %d", port);
            }
        }
    }
}

void CDownlinkSchedulerTenant::LandedRun() {
    if (!gnssDownlinkAvailable) {
        return;
    }

    auto gnssMessagePort = telemetryMessagePortMap.Get(NNetworkDefs::RADIO_MODULE_GNSS_DATA_PORT).value();
    auto gnssTimer = telemetryDownlinkTimers.GetPtr(NNetworkDefs::RADIO_MODULE_GNSS_DATA_PORT)->get();

    if (gnssTimer->IsExpired()) {
        LaunchLoraFrame telemFrame{};
        int ret = gnssMessagePort->Receive(telemFrame, K_NO_WAIT);
        if (ret < 0) {
            LOG_ERR("Failed to receive GNSS telemetry frame");
            return;
        }

        ret = loraDownlinkMessagePort.Send(telemFrame, K_NO_WAIT);
        if (ret < 0) {
            LOG_ERR("Failed to send GNSS telemetry frame");
        }
    }
}

void CDownlinkSchedulerTenant::GroundRun() {
    return;
}
