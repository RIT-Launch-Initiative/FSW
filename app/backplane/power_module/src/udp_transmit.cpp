#include "udp_transmit.h"

#include "f_core/device/sensor/c_sensor_device.h"
#include "zephyr/zbus/zbus.h"

#include <n_autocoder_types.h>
#include <string>
#include <f_core/utils/c_hashmap.h>
#include <f_core/net/transport/c_udp_socket.h>

void transmitRawData(const zbus_channel* chan) {
    CHashMap<std::string, void*> userData = *static_cast<CHashMap<std::string, void*>*>(zbus_chan_user_data(chan));
    CUdpSocket* socket = static_cast<CUdpSocket*>(userData.Get("TelemetrySocket").value_or(nullptr));
    if (socket == nullptr) {
        LOG_ERR("No socket was set for telemetry transmission");
        return;
    }

    NTypes::SensorData data = static_cast<const NTypes::TimestampedSensorData*>(zbus_chan_const_msg(chan))->data;
    socket->TransmitAsynchronous(&data, sizeof(NTypes::SensorData));
}

void transmitDownlinkData(const zbus_channel* chan) {
    CHashMap<std::string, void*> userData = *static_cast<CHashMap<std::string, void*>*>(zbus_chan_user_data(chan));
    CUdpSocket* socket = static_cast<CUdpSocket*>(userData.Get("DownlinkSocket").value_or(nullptr));
    if (socket == nullptr) {
        LOG_ERR("No socket was set for LoRa transmission");
        return;
    }

    const auto timestampedData = static_cast<const NTypes::TimestampedSensorData*>(zbus_chan_const_msg(chan));
    NTypes::LoRaBroadcastSensorData downlinkData{
        .RailBattery = {
            .Voltage = static_cast<int16_t>(CSensorDevice::ToMilliUnits(timestampedData->data.RailBattery.Voltage)),
            .Current = static_cast<int16_t>(CSensorDevice::ToMilliUnits(timestampedData->data.RailBattery.Current)),
            .Power = static_cast<int16_t>(CSensorDevice::ToMilliUnits(timestampedData->data.RailBattery.Power))
        },
        .Rail3v3 = {
            .Voltage = static_cast<int16_t>(CSensorDevice::ToMilliUnits(timestampedData->data.Rail3v3.Voltage)),
            .Current = static_cast<int16_t>(CSensorDevice::ToMilliUnits(timestampedData->data.Rail3v3.Current)),
            .Power = static_cast<int16_t>(CSensorDevice::ToMilliUnits(timestampedData->data.Rail3v3.Power))
        }
    };
    socket->TransmitAsynchronous(&downlinkData, sizeof(NTypes::LoRaBroadcastSensorData));
}
