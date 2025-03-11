#include "c_state_machine_updater.h"

#include <f_core/c_pad_flight_landing_state_machine.h>
#include <f_core/n_alerts.h>

void CStateMachineUpdater::Notify(void* ctx) {
    switch (*static_cast<NAlerts::AlertType*>(ctx)) {
        case NAlerts::BOOST:
            NStateMachineGlobals::boostDetected = true;
            break;

        case NAlerts::LANDED:
            NStateMachineGlobals::landingDetected = true;
            break;

        default:
            return;
    }
}


