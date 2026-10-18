#pragma once

#include "f_core/messaging/c_message_port.h"
#include "f_core/os/c_runnable_tenant.h"

#include <cstdint>
#include <zephyr/device.h>

struct __attribute__((packed)) CpuMonitorData {
    uint32_t Uptime;
    int32_t DieTemperature;
    uint8_t Utilization;
};

/**
 * Tenant that publishes uptime, CPU utilization and die temperature
 * Die temperature is read from the die-temp alias if it exists, otherwise it is reported as 0
 */
class CCpuMonitorTenant : public CRunnableTenant {
  public:
    /**
     * Constructor
     * @param outputPort Port to publish CPU statistics to
     */
    explicit CCpuMonitorTenant(CMessagePort<CpuMonitorData>& outputPort)
        : CRunnableTenant("CPU Monitor Tenant"), outputPort(outputPort) {}

    void Startup() override;

    void Run() override;

  private:
    CMessagePort<CpuMonitorData>& outputPort;

    // Zephyr's naming: execution_cycles is all cycles (active + idle), total_cycles is active cycles only
    uint64_t prevExecutionCycles = 0;
    uint64_t prevTotalCycles = 0;

#if defined(CONFIG_SENSOR) && DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(die_temp))
    const device* dieTempSensor = DEVICE_DT_GET(DT_ALIAS(die_temp));
#endif

    uint8_t getUtilization();

    int32_t getDieTemperature();
};
