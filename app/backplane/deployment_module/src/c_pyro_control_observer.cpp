#include "c_pyro_control_observer.h"

#include <cstdio>
#include <f_core/n_alerts.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/rtc.h>

LOG_MODULE_REGISTER(CPyroControlObserver);

CPyroControlObserver::CPyroControlObserver() {
    flightLog.Write("Pyro Controller Observer initialized");
}


void CPyroControlObserver::Notify(void* ctx) {
    uint8_t pyroCount = 0;

    switch (*static_cast<NAlerts::AlertType*>(ctx)) {
        case NAlerts::NOSEOVER:
            LOG_INF("Noseover detected. Deploying charges in one second.");
            flightLog.Write("Noseover detected. Deploying charges in one second.");
            // TODO: Settings library for handling deployment timing
            k_sleep(K_SECONDS(1));

            for (auto& [sense, ctrl] : pyroPairs) {
                if (sense.GetPin() == 1) {
                    ctrl.SetPin(1);
                    LOG_INF("Deployed charge %d", pyroCount);
                }
                pyroCount++;
            }
            flightLog.Write("Finished deploying charges");
            break;
        default:
            break;
    }

    flightLog.Sync();
}
