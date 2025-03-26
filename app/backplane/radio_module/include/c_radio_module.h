#ifndef CONFIG_RADIO_MODULE_RECEIVER

#ifndef C_RADIO_MODULE_H
#define C_RADIO_MODULE_H

#include "n_radio_module_types.h"
#include "c_gnss_tenant.h"
#include "c_udp_listener_tenant.h"
#include "c_lora_transmit_tenant.h"
#include "c_lora_receive_tenant.h"
#include "c_state_machine_updater.h"

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/net/application/c_tftp_server_tenant.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/net/application/c_sntp_server_tenant.h>
#include <f_core/net/application/c_udp_alert_tenant.h>
#include <f_core/os/c_task.h>
#include <f_core/os/tenants/c_datalogger_tenant.h>
#include <f_core/radio/c_lora.h>

// Autocoder Includes
#include <n_autocoder_network_defs.h>

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
    const device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

    static constexpr uint16_t powerModuleTelemetryPort = NNetworkDefs::POWER_MODULE_INA_DATA_PORT;
    static constexpr uint16_t radioModuleSourcePort = NNetworkDefs::RADIO_BASE_PORT;
    static constexpr uint16_t sensorModuleTelemetryPort = NNetworkDefs::SENSOR_MODULE_TELEMETRY_PORT;

    // Devices
#ifndef CONFIG_ARCH_POSIX
    CLora lora;
#endif

    // Message Ports
    CMessagePort<NTypes::RadioBroadcastData>& loraBroadcastMessagePort;
    CMessagePort<NTypes::RadioBroadcastData>& udpBroadcastMessagePort;
    CMessagePort<NTypes::GnssLoggingData>& gnssDataLogMessagePort;

    // Tenants
    CGnssTenant gnssTenant{"GNSS Tenant", &loraBroadcastMessagePort, &gnssDataLogMessagePort};

    CUdpListenerTenant sensorModuleListenerTenant{"Sensor Module Listener Tenant", ipAddrStr, sensorModuleTelemetryPort, &loraBroadcastMessagePort};
    CUdpListenerTenant powerModuleListenerTenant{"Power Module Listener Tenant", ipAddrStr, powerModuleTelemetryPort, &loraBroadcastMessagePort};

    CUdpAlertTenant alertTenant{"Alert Tenant", ipAddrStr, NNetworkDefs::ALERT_PORT};

#ifndef CONFIG_ARCH_POSIX
    CLoraTransmitTenant loraTransmitTenant{"LoRa Transmit Tenant", lora, &loraBroadcastMessagePort};
    CLoraReceiveTenant loraReceiveTenant{"LoRa Receive Tenant", loraTransmitTenant, ipAddrStr, radioModuleSourcePort};
#endif
    CDataLoggerTenant<NTypes::GnssLoggingData> dataLoggerTenant{"Data Logger Tenant", "/lfs/gps_data.bin", LogMode::Growing, 0, gnssDataLogMessagePort};
    CSntpServerTenant sntpServerTenant = *CSntpServerTenant::GetInstance(*rtc, CIPv4(ipAddrStr));


    CStateMachineUpdater stateMachineUpdater;

    // Tasks
    CTask networkingTask{"Networking Task", 14, 3072, 0};
    CTask gnssTask{"GNSS Task", 15, 1024, 2000};

    CTask dataLoggingTask{"Data Logging Task", 15, 2048, 0};
    CTask loraTask{"LoRa Task", 15, 2048, 0};

};

#endif //C_RADIO_MODULE_H
#endif //CONFIG_RADIO_MODULE_RECEIVER