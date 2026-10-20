#include "f_core/os/tenants/c_cpu_monitor_tenant.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CCpuMonitorTenant);

void CCpuMonitorTenant::Startup() {
    k_thread_runtime_stats stats{};
    k_thread_runtime_stats_all_get(&stats);

    prevExecutionCycles = stats.execution_cycles;
    prevTotalCycles = stats.total_cycles;
}

void CCpuMonitorTenant::Run() {
    CpuMonitorData cpuMonitorData{};

    cpuMonitorData.Uptime = k_uptime_get_32();
    cpuMonitorData.Utilization = getUtilization();
    cpuMonitorData.DieTemperature = getDieTemperature();

    if (outputPort.Send(cpuMonitorData, K_NO_WAIT) != 0) {
        outputPort.Clear();
    }
}

int32_t CCpuMonitorTenant::getDieTemperature() {
#if defined(CONFIG_SENSOR) && DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(die_temp))
    sensor_value dieTempValue{};
    if (device_is_ready(dieTempSensor) && sensor_sample_fetch(dieTempSensor) == 0 &&
        sensor_channel_get(dieTempSensor, SENSOR_CHAN_DIE_TEMP, &dieTempValue) == 0) {
        return dieTempValue.val1;
    }
#endif
    return 0;
}

uint8_t CCpuMonitorTenant::getUtilization() {
    k_thread_runtime_stats stats{};
    k_thread_runtime_stats_all_get(&stats);

    uint64_t deltaAllCycles = stats.execution_cycles - prevExecutionCycles;
    uint64_t deltaActiveCycles = stats.total_cycles - prevTotalCycles;

    prevExecutionCycles = stats.execution_cycles;
    prevTotalCycles = stats.total_cycles;

    if (deltaAllCycles == 0) {
        return 0;
    }

    uint64_t utilization = (deltaActiveCycles * 100) / deltaAllCycles;
    return static_cast<uint8_t>(utilization > 100 ? 100 : utilization);
}
