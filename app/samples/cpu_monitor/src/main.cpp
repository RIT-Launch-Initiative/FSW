#include <f_core/messaging/c_msgq_message_port.h>
#include <f_core/os/tenants/c_cpu_monitor_tenant.h>
#include <zephyr/kernel.h>

K_MSGQ_DEFINE(cpuMonitorQueue, sizeof(CpuMonitorData), 1, 4);

int main() {
    static auto cpuMonitorMsgQueue = CMsgqMessagePort<CpuMonitorData>(cpuMonitorQueue);
    static CCpuMonitorTenant cpuMonitorTenant(cpuMonitorMsgQueue);

    cpuMonitorTenant.Startup();

    while (true) {
        CpuMonitorData cpuMonitorData{};

        // Busy wait for a bit so there is some utilization to report
        k_busy_wait(250000);
        k_msleep(750);

        cpuMonitorTenant.Run();
        cpuMonitorMsgQueue.Receive(cpuMonitorData, K_FOREVER);
        printk("Uptime: %u ms, Utilization: %u%%, Die Temperature: %d C\n", cpuMonitorData.Uptime,
               cpuMonitorData.Utilization, cpuMonitorData.DieTemperature);
    }

    return 0;
}
