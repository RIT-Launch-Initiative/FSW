#ifndef C_RECEIVER_MODULE_H
#define C_RECEIVER_MODULE_H

#include "c_lora_receive_tenant.h"
#include "c_udp_listener_tenant.h"

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_task.h>
#include <f_core/radio/c_lora.h>

#include <n_autocoder_network_defs.h>

#include "f_core/radio/c_lora_link.h"
#include "f_core/radio/c_lora_tenant.h"

class CReceiverModule : public CProjectConfiguration {
public:
    /**
     * Constructor
     */
    CReceiverModule();

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
    std::string ipAddrStr = CREATE_IP_ADDR(NNetworkDefs::RADIO_MODULE_IP_ADDR_BASE, 1, 1);
    static constexpr uint16_t radioModuleCommandPort = NNetworkDefs::RADIO_MODULE_COMMAND_PORT;
    static constexpr uint16_t radioModuleDataRequestPort = NNetworkDefs::RADIO_MODULE_DATA_REQUEST_PORT;

    // Devices
    CLora lora;

    // Message Ports
    CMessagePort<LaunchLoraFrame>& loraBroadcastMessagePort;
    CMessagePort<LaunchLoraFrame>& udpBroadcastMessagePort;

    // Tenants
    CLoraLink loraLink{lora};
    CLoraTenant loraTenant{loraLink, loraBroadcastMessagePort};

    CUdpListenerTenant commandListenerTenant{"Radio Module Command Listener Tenant", ipAddrStr.c_str(), radioModuleCommandPort, &loraBroadcastMessagePort};
    CUdpListenerTenant dataRequestListenerTenant{"Radio Module Data Request Listener Tenant", ipAddrStr.c_str(), radioModuleDataRequestPort, &loraBroadcastMessagePort};

    // Tasks
    CTask networkingTask{"UDP Listener Task", 15, 4096, 0};
    CTask loraTask{"LoRa Task", 15, 4096, 0};
};

#endif //C_RECEIVER_MODULE_H
