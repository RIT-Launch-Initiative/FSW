#include "c_shunt_monitor.h"

#include <f_core/device/sensor/c_sensor_device.h>

void CShuntMonitor::Run() {
    ina3v3.UpdateSensorValue();
    ina5v0.UpdateSensorValue();
    inaBatt.UpdateSensorValue();

    // TODO: Need a types file to create a telemetry message
}