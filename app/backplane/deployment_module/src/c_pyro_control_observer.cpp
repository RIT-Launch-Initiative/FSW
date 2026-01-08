#include "c_pyro_control_observer.h"

#include "f_core/os/n_rtos.h"

#include <cstdio>
#include <f_core/n_alerts.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/rtc.h>

LOG_MODULE_REGISTER(CPyroControlObserver);

static void chargeDisableTimerCallback(k_timer* timer) {
    auto* observer = static_cast<CPyroControlObserver*>(timer->user_data);

    observer->DisableCallback();
    NRtos::ResumeTask("Networking Task");
}

CPyroControlObserver::CPyroControlObserver() : chargeDisableTimer(chargeDisableTimerCallback) {
    // flightLog.Write("Pyro Controller Observer initialized");
    chargeDisableTimer.SetUserData(this);
}


void CPyroControlObserver::Notify(void* ctx) {
    uint8_t pyroCount = 0;
    LOG_INF("Notified");

    switch (*static_cast<NAlerts::AlertType*>(ctx)) {
        case NAlerts::NOSEOVER:
            LOG_INF("Noseover detected. Deploying charges in one second.");
            // flightLog.Write("Noseover detected. Deploying charges in one second.");
            // TODO: Settings library for handling deployment timing

            for (auto& [sense, ctrl, led] : pyroTrios) {
                // if (sense.GetPin() == 1) {
                    ctrl.SetPin(1);
                    led.SetPin(1);
                    LOG_INF("Deployed charge %d", pyroCount);
                // }
                pyroCount++;
            }
            // flightLog.Write("Finished deploying charges");
            chargeDisableTimer.StartTimer(3000);
            // Must suspend the networking task. If we get a new event, process after we
            // process the last one. Otherwise, we can fault during interrupt handling
            NRtos::SuspendCurrentTask();
            break;
        default:
            break;
    }


    // flightLog.Sync();
}

void CPyroControlObserver::DisableCallback() {
    chargeDisableTimer.StopTimer();
    int pyroCount = 0;
    for (auto& [sense, ctrl, led] : pyroTrios) {
        ctrl.SetPin(0);
        led.SetPin(0);
        LOG_INF("Disabled charge %d", pyroCount);
        pyroCount++;
    }
    // flightLog.Write("Finished disabling charges");
}
