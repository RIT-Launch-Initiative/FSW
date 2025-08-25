#include "f_core/os/tenants/c_cpu_monitor_tenant.h"

#include "zephyr/logging/log.h"

#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(CCpuMonitorTenant);

void CCpuMonitorTenant::Startup() {
    k_thread_runtime_stats stats{0};
    k_thread_runtime_stats_all_get(&stats);

    prevExecutionCycles = stats.execution_cycles;
    prevTotalCycles = stats.total_cycles;
    prevUptime = k_uptime_get_32();
}

void CCpuMonitorTenant::PostStartup() {
}

void CCpuMonitorTenant::Run() {
    CpuMonitorData cpuMonitorData{0};

    cpuMonitorData.Uptime = getUptime();
    cpuMonitorData.Utilization = getUtilization();
    cpuMonitorData.DieTemperature = getDieTemperature();

    if (outputPort.Send(cpuMonitorData, K_NO_WAIT) != 0) {
        outputPort.Clear();
    }
}

void CCpuMonitorTenant::Cleanup() {
}

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
    uint32_t currentUptime = k_uptime_get_32();

    // Zephyr's naming is confusing! Based on kernel/thread.h comments:
    // execution_cycles = total # of cycles (cpu: non-idle + idle) = ALL cycles
    // total_cycles = total # of non-idle cycles = ACTIVE/BUSY cycles only
    uint64_t deltaAllCycles = stats.execution_cycles - prevExecutionCycles;  // All cycles (active + idle)
    uint64_t deltaActiveCycles = stats.total_cycles - prevTotalCycles;       // Active cycles only
    uint32_t deltaTime = currentUptime - prevUptime;

    prevExecutionCycles = stats.execution_cycles;
    prevTotalCycles = stats.total_cycles;
    prevUptime = currentUptime;

    if (deltaAllCycles == 0 || deltaTime == 0) {
        return 0; // Avoid division by zero
    }

    if (deltaTime < 10) {
        LOG_WRN_ONCE("CPU utilization measurement interval too short for accuracy");
    }

    // CPU Utilization = (Active cycles / All cycles) × 100
    // gives the percentage of time the CPU is NOT idle
    uint64_t utilization = (deltaActiveCycles * 100) / deltaAllCycles;
    
    // Clamp to 100% in case of any edge cases
    return static_cast<uint8_t>(utilization > 100 ? 100 : utilization);
}
