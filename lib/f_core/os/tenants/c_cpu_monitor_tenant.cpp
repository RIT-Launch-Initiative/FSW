#include "f_core/os/tenants/c_cpu_monitor_tenant.h"

#include "zephyr/logging/log.h"

#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(CCpuMonitorTenant);

void CCpuMonitorTenant::Startup() {
    k_thread_runtime_stats stats{0};
    k_thread_runtime_stats_all_get(&stats);

    prevExecutionCycles = stats.execution_cycles;
    prevTotalCycles = stats.total_cycles;
}

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
#else
    LOG_WRN_ONCE("No die temperature sensor available. Defaulting to 0 °C.");
#endif
    return result;
}

uint32_t CCpuMonitorTenant::getUptime() {
    return k_uptime_get_32();
}

uint8_t CCpuMonitorTenant::getUtilization() {
    k_thread_runtime_stats stats{0};

    k_thread_runtime_stats_all_get(&stats);
    if (stats.execution_cycles == 0) {
        return 0; // Avoid division by zero
    }


    // Note that total cycles is non-idle cycles
    // and execution cycles is the sum of non-idle + idle cycles.
    return static_cast<uint8_t>(
        ((stats.total_cycles) * 100) / (stats.execution_cycles));
}
