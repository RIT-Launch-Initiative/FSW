#include "c_gnss_tenant.h"

#include "c_radio_module.h"

#include <f_core/utils/n_gnss_utils.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CGnssTenant);

static NGnssUtils::GnssCoordinates coordinates;
static uint8_t gnssUpdated = 0;

static void gnssCallback(const device *, const gnss_data *data) {
    PopulateGnssNavigationData(&data->nav_data, &coordinates);
    gnssUpdated = 1;
    LOG_INF("Latitude: %f, Longitude: %f, Altitude: %f", static_cast<double>(coordinates.latitude),
            static_cast<double>(coordinates.longitude), static_cast<double>(coordinates.altitude));
}

GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), gnssCallback);

void CGnssTenant::Startup() { transmitTimer.StartTimer(5000); }

void CGnssTenant::PostStartup() {}

void CGnssTenant::Run() {
    NRadioModuleTypes::GnssBroadcastData gnssData{0};
    NRadioModuleTypes::RadioBroadcastData broadcastData{0};
    if (transmitTimer.IsExpired()) {
        memcpy(&gnssData.coordinates, &coordinates, sizeof(NGnssUtils::GnssCoordinates));
        gnssData.updated = gnssUpdated;
        gnssUpdated = 0;

        broadcastData.port = 12000;
        broadcastData.size = sizeof(NRadioModuleTypes::GnssBroadcastData);
        memcpy(broadcastData.data, &gnssData, sizeof(NRadioModuleTypes::GnssBroadcastData));
        loraTransmitPort.Send(broadcastData);
    }
}
