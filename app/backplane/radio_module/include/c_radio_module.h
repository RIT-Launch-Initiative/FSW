#ifndef C_RADIO_MODULE_H
#define C_RADIO_MODULE_H

#include "c_gnss_tenant.h"
#include "c_udp_listener_tenant.h"
#include "c_lora_transmit_tenant.h"
#include "c_lora_receive_tenant.h"
#include "c_state_machine_updater.h"

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/net/application/c_sntp_server_tenant.h>
#include <f_core/net/application/c_udp_alert_tenant.h>
#include <f_core/os/c_task.h>
#include <f_core/os/tenants/c_datalogger_tenant.h>
#include <f_core/radio/c_lora.h>
#include <f_core/device/c_rtc.h>

// Autocoder Includes
#include <n_autocoder_network_defs.h>
#include <n_autocoder_types.h>

class CRadioModule : public CProjectConfiguration {
public:
    /**
     * Constructor
     */
    CRadioModule();

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

    static rtc_time lastGnssUpdateTime;

private:
    const char* ipAddrStr = (CREATE_IP_ADDR(NNetworkDefs::RADIO_MODULE_IP_ADDR_BASE, 1, CONFIG_MODULE_ID)).c_str();

    static constexpr uint16_t powerModuleTelemetryPort = NNetworkDefs::POWER_MODULE_DOWNLINK_DATA_PORT;
    static constexpr uint16_t radioModuleSourcePort = NNetworkDefs::RADIO_BASE_PORT;
    static constexpr uint16_t sensorModuleTelemetryPort = NNetworkDefs::SENSOR_MODULE_DOWNLINK_DATA_PORT;

    // Devices
#ifndef CONFIG_ARCH_POSIX
    CLora lora;
#endif
    CRtc rtc{*DEVICE_DT_GET(DT_ALIAS(rtc))};

    // Message Ports
    CMessagePort<NTypes::LoRaBroadcastData>& loraBroadcastMessagePort;
    CMessagePort<NTypes::LoRaBroadcastData>& udpBroadcastMessagePort;
    CMessagePort<NTypes::GnssData>& gnssDataLogMessagePort;

    // Tenants
    CGnssTenant gnssTenant{"GNSS Tenant", &loraBroadcastMessagePort, &gnssDataLogMessagePort};

    CUdpListenerTenant sensorModuleListenerTenant{"Sensor Module Listener Tenant", ipAddrStr, sensorModuleTelemetryPort, &loraBroadcastMessagePort};
    CUdpListenerTenant powerModuleListenerTenant{"Power Module Listener Tenant", ipAddrStr, powerModuleTelemetryPort, &loraBroadcastMessagePort};
    CSntpServerTenant sntpServerTenant = *CSntpServerTenant::GetInstance(rtc, CIPv4(ipAddrStr));
    CUdpAlertTenant alertTenant{"Alert Tenant", ipAddrStr, NNetworkDefs::ALERT_PORT};

#ifndef CONFIG_ARCH_POSIX
    CLoraTransmitTenant loraTransmitTenant{"LoRa Transmit Tenant", lora, &loraBroadcastMessagePort};
    CLoraReceiveTenant loraReceiveTenant{"LoRa Receive Tenant", loraTransmitTenant, ipAddrStr, radioModuleSourcePort};
#endif
    CDataLoggerTenant<NTypes::GnssData> dataLoggerTenant{"Data Logger Tenant", "/lfs/gps_data.bin", LogMode::Growing, 0, gnssDataLogMessagePort};
    CStateMachineUpdater stateMachineUpdater;

    // Tasks
    CTask networkingTask{"Networking Task", 14, 3072, 5};
    CTask gnssTask{"GNSS Task", 15, 1024, 2000};
    CTask dataLoggingTask{"Data Logging Task", 15, 2048, 10};
    CTask loraTask{"LoRa Task", 14, 2048, 10};

};

#endif //C_RADIO_MODULE_H
