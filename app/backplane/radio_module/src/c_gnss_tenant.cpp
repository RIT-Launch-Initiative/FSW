#include "c_gnss_tenant.h"
#include "c_radio_module.h"

#include <f_core/utils/n_gnss_utils.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CGnssTenant);

static NTypes::GnssLoggingData gnssLogData{0};
static NGnssUtils::GnssCoordinates coordinates{0};
static uint8_t gnssUpdated = 0;

static void gnssCallback(const device *, const gnss_data *data) {
    LOG_INF("Triggered");
    gnssLogData.systemTime = k_uptime_get();
    PopulateGnssStruct(data, &gnssLogData.gnssData);

    gnssUpdated = 1;
    memcpy(&coordinates, &gnssLogData.gnssData.coordinates, sizeof(NGnssUtils::GnssCoordinates));

    LOG_INF("Latitude: %f, Longitude: %f, Altitude: %f",
        static_cast<double>(coordinates.latitude),
        static_cast<double>(coordinates.longitude),
        static_cast<double>(coordinates.altitude));
}

GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), gnssCallback);

void CGnssTenant::Startup() {
    transmitTimer.StartTimer(5000);
}

void CGnssTenant::PostStartup() {

}

void CGnssTenant::Run() {
    NTypes::RadioBroadcastData broadcastData{0};
    NTypes::GnssLoggingData logData{0};

    if (gnssUpdated) {
        memcpy(&logData, &gnssLogData, sizeof(NTypes::GnssLoggingData));
        dataLoggingPort.Send(logData);

        if (transmitTimer.IsExpired()) {
            broadcastData.port = 12000;
            broadcastData.size = sizeof(NTypes::GnssBroadcastData);
            memcpy(broadcastData.data, &coordinates, sizeof(NGnssUtils::GnssCoordinates));
            reinterpret_cast<NTypes::GnssBroadcastData*>(broadcastData.data)->updated = 1;
            loraTransmitPort.Send(broadcastData);
        }

        gnssUpdated = 0;
    }


}
