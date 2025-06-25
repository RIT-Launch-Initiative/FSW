#include <f_core/os/tenants/c_cpu_monitor_tenant.h>
#include <f_core/messaging/c_msgq_message_port.h>
#include <n_autocoder_types.h>
#include <zephyr/kernel.h>

K_MSGQ_DEFINE(cpuMonitorQueue, sizeof(CpuMonitorData), 1, 4);

int main() {
    auto cpuMonitorMsgQueue = CMsgqMessagePort<CpuMonitorData>(cpuMonitorQueue);
    CCpuMonitorTenant cpuMonitorTenant(cpuMonitorMsgQueue);

    cpuMonitorTenant.Startup();

    while (true) {
        CpuMonitorData cpuMonitorData{0, 0, 0};
        cpuMonitorTenant.Run();
        cpuMonitorMsgQueue.Receive(cpuMonitorData, K_FOREVER);
        printk("Uptime: %u ms, Utilization: %u%%, Die Temperature: %d °C\n",
               cpuMonitorData.Uptime, cpuMonitorData.Utilization, cpuMonitorData.DieTemperature);
        k_msleep(1000);
    }

    return 0;
}
