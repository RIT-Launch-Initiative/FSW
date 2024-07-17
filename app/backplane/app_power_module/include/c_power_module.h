#ifndef C_POWER_MODULE_H
#define C_POWER_MODULE_H

// Tenant Includes
#include "c_adc_monitor.h"
#include "c_shunt_monitor.h"

// F-Core Includes
#include <f_core/os/c_task.h>

class CPowerModule {
public:
    CPowerModule() = default;

    void Initialize();

private:
    CTask telemetryTask{"Telemetry Task", 15, 512};
    CTask broadcastTask{"Broadcast Task", 15, 512};
    CTask dataLogTask{"Data Logging Task", 15, 512};

    // CTenant *shuntMonitorTenant{"Shunt Monitor"};
    // CTenant *adcMonitorTenant{"ADC Monitor"};
    // CTenant *telemetryBroadcastTenant{"Telemetry Broadcast"};
    // CTenant *dataLogTenant{"Data Logging"};

    // TODO: OMG FORGOT THE SHUNT SENSOR CLASS
    const device &ina3v3 = *DEVICE_DT_GET(DT_ALIAS(ina3v3));
    const device &ina5v0 = *DEVICE_DT_GET(DT_ALIAS(ina5v0));
    const device &inaBatt = *DEVICE_DT_GET(DT_ALIAS(inabatt));
    void addTenantsToTasks();
};



#endif //C_POWER_MODULE_H
