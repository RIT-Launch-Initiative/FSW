#include "c_pyro_control_observer.h"

#include <cstdio>
#include <f_core/n_alerts.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/rtc.h>

LOG_MODULE_REGISTER(CPyroControlObserver);

void CPyroControlObserver::logTimestamp() {
    static const device* rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
    static uint8_t timestampBuff[100]{0};
    rtc_time time{0};

    rtc_get_time(rtc, &time);
    snprintf(reinterpret_cast<char*>(timestampBuff), sizeof(timestampBuff), "%02d-%02d %02d:%02d:%02d.%03d",
             time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, time.tm_nsec / 1000000);
    logFile.Write(timestampBuff, sizeof(timestampBuff));
}

void CPyroControlObserver::Notify(void* ctx) {
    uint8_t pyroCount = 0;
    uint8_t pyroCountStr[2]{0}; // 2 to deal with truncation warnings

    switch (*static_cast<NAlerts::AlertType*>(ctx)) {
        case NAlerts::NOSEOVER:
            LOG_INF("Noseover detected. Deploying charges.");
            logTimestamp();
            logFile.Write(" - Noseover detected. Deploying charges.\n", 40);
            // TODO: Settings library for handling deployment timing


            for (auto& [sense, ctrl] : pyroPairs) {
                if (sense.GetPin() == 1) {
                    logTimestamp();

                    snprintf(reinterpret_cast<char*>(pyroCountStr), 2, "%d", pyroCount);
                    logFile.Write(pyroCountStr, 1);
                    logFile.Write(" - Deployed pyro ", 17);

                    ctrl.SetPin(1);
                }
                pyroCount++;
            }

            break;
        default:
            break;
    }
}
