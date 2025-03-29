#include "c_pyro_control_observer.h"

#include <cstdio>
#include <f_core/n_alerts.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/rtc.h>

LOG_MODULE_REGISTER(CPyroControlObserver);

static void chargeDisableTimerCallback(k_timer* timer) {
    auto* observer = static_cast<CPyroControlObserver*>(timer->user_data);
    observer->DisableCallback();
}

CPyroControlObserver::CPyroControlObserver() {
    flightLog.Write("Pyro Controller Observer initialized");
    chargeDisableTimer = CSoftTimer(chargeDisableTimerCallback);
    chargeDisableTimer.SetUserData(this);
}


void CPyroControlObserver::Notify(void* ctx) {
    uint8_t pyroCount = 0;

    switch (*static_cast<NAlerts::AlertType*>(ctx)) {
        case NAlerts::NOSEOVER:
            LOG_INF("Noseover detected. Deploying charges in one second.");
            flightLog.Write("Noseover detected. Deploying charges in one second.");
            // TODO: Settings library for handling deployment timing
            k_sleep(K_SECONDS(1));

            for (auto& [sense, ctrl, led] : pyroTrios) {
                if (sense.GetPin() == 1) {
                    ctrl.SetPin(1);
                    led.SetPin(1);
                    LOG_INF("Deployed charge %d", pyroCount);
                }
                pyroCount++;
            }
            flightLog.Write("Finished deploying charges");
            chargeDisableTimer.StartTimer(3000);
            break;
        default:
            break;
    }

    flightLog.Sync();
}

void CPyroControlObserver::DisableCallback() {
    int pyroCount = 0;
    for (auto& [sense, ctrl, led] : pyroTrios) {
        ctrl.SetPin(0);
        led.SetPin(0);
        LOG_INF("Disabled charge %d", pyroCount);
    }
    flightLog.Write("Finished disabling charges");
}
