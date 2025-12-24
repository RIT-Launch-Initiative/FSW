#include "c_downlink_scheduler_tenant.h"

void CDownlinkSchedulerTenant::HandleFrame(const LaunchLoraFrame& frame) {
    if (this->state == State::FLIGHT) {


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
