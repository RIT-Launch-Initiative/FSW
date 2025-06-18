#include "f_core/os/tenants/c_cpu_monitor_tenant.h"

#include <zephyr/drivers/sensor.h>

void CCpuMonitorTenant::Startup() {}

void CCpuMonitorTenant::PostStartup() {}

void CCpuMonitorTenant::Run() {
    NTypes::CPUMonitor cpuMonitorData{0};

    cpuMonitorData.Uptime = getUptime();
    cpuMonitorData.Utilization = getUtilization();
    cpuMonitorData.DieTemperature = getDieTemperature();

    if (outputPort.Send(cpuMonitorData, K_NO_WAIT) != 0) {
        outputPort.Clear();
    }
}

void CCpuMonitorTenant::Cleanup() {}

int32_t CCpuMonitorTenant::getDieTemperature() {
    int32_t result = 0;
#if DT_NODE_EXISTS(DT_ALIAS(die_temp))
    if (device_is_ready(dieTempSensor)) {
        sensor_value dieTempValue{0};
        if (sensor_sample_fetch(dieTempSensor) == 0 &&
            sensor_channel_get(dieTempSensor, SENSOR_CHAN_DIE_TEMP, &dieTempValue) == 0) {
            result = dieTempValue.val1;
        }
    }
#endif
    return result;
}

uint32_t CCpuMonitorTenant::getUptime() {
    return k_uptime_get_32();
}

uint8_t CCpuMonitorTenant::getUtilization() {
    k_thread_runtime_stats stats{0};

    k_thread_runtime_stats_all_get(&stats);
    if (stats.total_cycles == 0) {
        return 0; // Avoid division by zero
    }

    return static_cast<uint8_t>((stats.execution_cycles * 100) / stats.total_cycles);
}
