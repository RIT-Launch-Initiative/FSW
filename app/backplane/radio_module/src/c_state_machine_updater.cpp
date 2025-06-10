#include "c_state_machine_updater.h"

#include <../../../../include/f_core/state_machine/c_pad_flight_landing_state_machine.h>
#include <f_core/n_alerts.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CStateMachineUpdater);

void CStateMachineUpdater::Notify(void* ctx) {
    switch (*static_cast<NAlerts::AlertType*>(ctx)) {
        case NAlerts::BOOST:
            k_mutex_lock(&NStateMachineGlobals::boostMutex, K_FOREVER);
            NStateMachineGlobals::boostDetected = true;
            k_mutex_unlock(&NStateMachineGlobals::boostMutex);
            break;

        case NAlerts::LANDED:
            k_mutex_lock(&NStateMachineGlobals::landingMutex, K_FOREVER);
            NStateMachineGlobals::landingDetected = true;
            k_mutex_unlock(&NStateMachineGlobals::landingMutex);
            break;

        default:
            return;
    }
}


