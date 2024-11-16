#include "c_gnss_tenant.h"
#include "c_radio_module.h"

#include <f_core/utils/n_gnss_utils.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CGnssTenant);


static NGnssUtils::GnssCoordinates coordinates;
static uint8_t gnssUpdated = 0;

static void gnssCallback(const device *dev, const gnss_data *data) {
    PopulateGnssNavigationData(&data->nav_data, &coordinates);
    gnssUpdated = 1;
    LOG_INF("Latitude: %f, Longitude: %f, Altitude: %f", coordinates.latitude, coordinates.longitude, coordinates.altitude);
}


GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), gnssCallback);

// TODO: Might want to implement a soft timer class in another PR, so we can take advantage of GNSS callbacks

void CGnssTenant::Startup() {
    transmitTimer.StartTimer(5000);
}

void CGnssTenant::PostStartup() {

}

void CGnssTenant::Run() {
    NRadioModuleTypes::GnssBroadcastData data{0};
    if (transmitTimer.IsExpired()) {
        memcpy(&data.coordinates, &coordinates, sizeof(NGnssUtils::GnssCoordinates));
        data.updated = gnssUpdated;
        gnssUpdated = 0;




    }
}
