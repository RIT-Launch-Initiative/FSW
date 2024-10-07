#ifndef C_RADIO_MODULE_H
#define C_RADIO_MODULE_H

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_task.h>
#include <f_core/types.h>
#include <f_core/net/application/c_udp_broadcast_tenant.h>

class CRadioModule : public CProjectConfiguration {
public:
    /**
     * Constructor
     */
    CRadioTransmitter();

    /**
     * See parent docs
     */
    void AddTenantsToTasks() override;

    /**
     * See parent docs
     */
    void AddTasksToRtos() override;

    /**
     * See parent docs
     */
    void SetupCallbacks() override;

private:
    static constexpr const char* ipAddrStr = "10.2.1.1";
    static constexpr int telemetryBroadcastPort = 12000;

    typedef struct {
        uint16_t port;
        uint8_t data[255 - sizeof(uint16_t)];
    } LoraPacket;

    // Tenants
    CMsgqMessagePort<LoraPacket> loraPacketToUdpPort{};
    CUdpBroadcastTenant<SensorData> broadcastTenant{"Broadcast Tenant", ipAddrStr, telemetryBroadcastPort, telemetryBroadcastPort, sensorDataBroadcastMessagePort};

    // Tasks
    CTask networkTask{"Networking Task", 15, 128, 0};
};

#endif //C_RADIO_MODULE_H
