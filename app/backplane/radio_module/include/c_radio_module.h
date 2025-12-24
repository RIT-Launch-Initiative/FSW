#ifndef C_RADIO_MODULE_H
#define C_RADIO_MODULE_H

#include "c_gnss_tenant.h"
#include "c_udp_listener_tenant.h"
#include "c_state_machine_updater.h"
#include "c_remote_gpio_handler.h"
#include "c_downlink_scheduler_tenant.h"

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/messaging/c_latest_message_port.h>
#include <f_core/net/application/c_sntp_server_tenant.h>
#include <f_core/net/application/c_udp_alert_tenant.h>
#include <f_core/os/c_task.h>
#include <f_core/os/tenants/c_datalogger_tenant.h>
#include <f_core/radio/c_lora_router.h>
#include <f_core/radio/c_lora_tenant.h>
#include <f_core/radio/frame_handlers/c_lora_frame_to_udp_handler.h>
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
    std::string ipAddrStr = CREATE_IP_ADDR(NNetworkDefs::RADIO_MODULE_IP_ADDR_BASE, 1, CONFIG_MODULE_ID);

    static constexpr uint16_t radioModuleSourcePort = NNetworkDefs::RADIO_BASE_PORT;
    static constexpr uint16_t powerModuleTelemetryPort = NNetworkDefs::POWER_MODULE_DOWNLINK_DATA_PORT;
    static constexpr uint16_t sensorModuleTelemetryPort = NNetworkDefs::SENSOR_MODULE_DOWNLINK_DATA_PORT;
    static constexpr uint16_t gnssTelemetryPort = NNetworkDefs::RADIO_MODULE_GNSS_DATA_PORT;

    // Devices
#ifndef CONFIG_ARCH_POSIX
    CLora lora;
#endif
    CRtc rtc{*DEVICE_DT_GET(DT_ALIAS(rtc))};

    // Message Ports
    CMessagePort<LaunchLoraFrame>& loraDownlinkMessagePort;
    CMessagePort<NTypes::GnssData>& gnssDataLogMessagePort;
    CLatestMessagePort<LaunchLoraFrame> sensorModuleTelemetryMessagePort;
    CLatestMessagePort<LaunchLoraFrame> powerModuleTelemetryMessagePort;
    CLatestMessagePort<LaunchLoraFrame> gnssTelemetryMessagePort;


    // Build the hashamp here for passing into downlink scheduler tenant
    CHashMap<uint16_t, CMessagePort<LaunchLoraFrame>*> telemetryMessagePortMap = {
        {sensorModuleTelemetryPort, &sensorModuleTelemetryMessagePort},
        {powerModuleTelemetryPort, &powerModuleTelemetryMessagePort},
        {gnssTelemetryPort, &gnssTelemetryMessagePort}
    };

    // Build downlink timer map here so easy to add new telemetry along with above hashmap,
    // and have downlink scheduler figure out starting the timers itself
    CHashMap<uint16_t, k_timeout_t> telemetryDownlinkTimes = {
        {sensorModuleTelemetryPort, K_SECONDS(2)},
        {powerModuleTelemetryPort, K_SECONDS(10)},
        {gnssTelemetryPort, K_SECONDS(5)}
    };

    // Tenants
    CGnssTenant gnssTenant{"GNSS Tenant", &gnssTelemetryMessagePort, &gnssDataLogMessagePort};

    CUdpListenerTenant sensorModuleListenerTenant{
        "Sensor Module Listener Tenant",
        ipAddrStr.c_str(),
        sensorModuleTelemetryPort,
        &sensorModuleTelemetryMessagePort
    };

    CUdpListenerTenant powerModuleListenerTenant{
        "Power Module Listener Tenant", ipAddrStr.c_str(),
        powerModuleTelemetryPort,
        &powerModuleTelemetryMessagePort
    };

    CSntpServerTenant sntpServerTenant = *CSntpServerTenant::GetInstance(rtc, CIPv4(ipAddrStr.c_str()));

    CDownlinkSchedulerTenant downlinkSchedulerTenant{
        loraDownlinkMessagePort,
        telemetryMessagePortMap,
        telemetryDownlinkTimes
    };

    CUdpAlertTenant alertTenant{"Alert Tenant", ipAddrStr.c_str(), NNetworkDefs::ALERT_PORT};

#ifndef CONFIG_ARCH_POSIX
    CLoraTenant loraTenant{lora, loraDownlinkMessagePort};
#endif
    CDataLoggerTenant<NTypes::GnssData> dataLoggerTenant{
        "Data Logger Tenant", "/lfs/gps_data.bin", LogMode::Growing, 0, gnssDataLogMessagePort, K_SECONDS(15), 5
    };

    CStateMachineUpdater stateMachineUpdater;

    // LoRa Handlers
    CRemoteGpioHandler remoteGpioHandler;
    CLoraFrameToUdpHandler loraToUdpHandler{
        CUdpSocket(CIPv4(ipAddrStr.c_str()), radioModuleSourcePort, radioModuleSourcePort),
        radioModuleSourcePort
    };

    // Tasks
    CTask networkingTask{"Networking Task", 14, 3072, 5};
    CTask gnssTask{"GNSS Task", 15, 1024, 2000};
    CTask dataLoggingTask{"Data Logging Task", 15, 2048, 10};
    CTask loraTask{"LoRa Task", 14, 2048, 10};
};

#endif //C_RADIO_MODULE_H
