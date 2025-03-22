#include "c_deployment_control_tenant.h"

#include <cstdio>
#include <f_core/n_alerts.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/rtc.h>

LOG_MODULE_REGISTER(CDeploymentControlObserver);

void CDeploymentControlObserver::logTimestamp() {
    static const device* rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
    static uint8_t timestampBuff[20]{0};
    rtc_time time{0};

    rtc_get_time(rtc, &time);
    snprintf(reinterpret_cast<char*>(timestampBuff), sizeof(timestampBuff), "%02d-%02d %02d:%02d:%02d.%03d",
             time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, time.tm_nsec / 1000000);
    logFile.Write(timestampBuff, sizeof(timestampBuff));
}

void CDeploymentControlObserver::Notify(void* ctx) {

    switch (*static_cast<NAlerts::AlertType*>(ctx)) {
        case NAlerts::NOSEOVER:
            LOG_INF("Noseover detected. Deploying charges.");
            logTimestamp();
            logFile.Write(" - Noseover detected. Deploying charges.\n", 40);
            // TODO: Settings library for handling deployment timing

            uint8_t pyroCount = 0;
            uint8_t pyroCountStr[1];
            for (auto& [sense, ctrl] : pyroPairs) {
                if (sense.GetPin() == 1) {
                    logTimestamp();

                    snprintf(reinterpret_cast<char*>(pyroCountStr), 1, "%d", pyroCount);
                    logFile.Write(pyroCountStr, 1);
                    logFile.Write(" - Deployed pyro ", 17);

                    ctrl.SetPin(1);
                }
                pyroCount++;
            }

            break;
    }
}
