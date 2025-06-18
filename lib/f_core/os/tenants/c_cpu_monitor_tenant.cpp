#include "f_core/os/tenants/c_cpu_monitor_tenant.h"

#include "zephyr/drivers/sensor.h"


void CCpuMonitorTenant::Startup() {}
void CCpuMonitorTenant::PostStartup() {}
void CCpuMonitorTenant::Run() {
    NTypes::CPUMonitor cpuMonitorData{0};

    cpuMonitorData.Uptime = k_uptime_get_32();

#if DT_NODE_EXISTS(DT_ALIAS(die_temp))
    if (device_is_ready(dieTempSensor)) {
        sensor_value dieTempValue{0};
        if (sensor_sample_fetch(dieTempSensor) == 0 &&
            sensor_channel_get(dieTempSensor, SENSOR_CHAN_DIE_TEMP, &dieTempValue) == 0) {
            cpuMonitorData.DieTemperature = dieTempValue.val1;
        }
    }
#endif

    if (outputPort.Send(cpuMonitorData, K_NO_WAIT) != 0) {
        outputPort.Clear();
    }
}
void CCpuMonitorTenant::Cleanup() {}
