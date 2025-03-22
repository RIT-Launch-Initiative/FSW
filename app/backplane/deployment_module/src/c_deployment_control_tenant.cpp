#include "c_deployment_control_tenant.h"

#include <f_core/n_alerts.h>
#include <zephyr/logging/log.h>


void CDeploymentControlObserver::Notify(void* ctx) {
    switch (*static_cast<NAlerts::AlertType*>(ctx)) {
        case NAlerts::NOSEOVER:
            LOG_INF("Noseover detected. Deploying charges.");
            for (auto& [sense, ctrl] : pyroPairs) {
                if (sense.GetPin() == 1) {
                    ctrl.SetPin(1);
                }
            }

            break;
    }
}
