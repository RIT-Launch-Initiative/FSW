#include "c_gnss_tenant.h"
#include "c_radio_module.h"

#include <f_core/utils/n_gnss_utils.h>
#include <zephyr/drivers/rtc.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CGnssTenant);

static NTypes::GnssLoggingData gnssLogData{0};
static NGnssUtils::GnssCoordinates coordinates{0};
static uint8_t gnssUpdated = 0;

static void gnssCallback(const device *, const gnss_data *data) {
    static const device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
    gnssLogData.systemTime = k_uptime_get();
    PopulateGnssStruct(data, &gnssLogData.gnssData);

    gnssUpdated = 1;
    memcpy(&coordinates, &gnssLogData.gnssData.coordinates, sizeof(NGnssUtils::GnssCoordinates));

    LOG_DBG("Latitude: %f, Longitude: %f, Altitude: %f",
        static_cast<double>(coordinates.latitude),
        static_cast<double>(coordinates.longitude),
        static_cast<double>(coordinates.altitude));

    // Set the rtc time
    rtc_time lastUpdated = {
        .tm_sec = data->utc.millisecond / 1000,
        .tm_min = data->utc.minute,
        .tm_hour = data->utc.hour,
        .tm_mday = data->utc.month_day,
        .tm_mon = data->utc.month,
        .tm_year = data->utc.century_year,
        .tm_wday = -1,
        .tm_yday = -1,
        .tm_isdst = -1,
        .tm_nsec = (data->utc.millisecond % 1000) * 1000000,
    };

    CSntpServerTenant::SetLastUpdatedTime(lastUpdated);
    rtc_set_time(rtc, &lastUpdated);
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
            broadcastData.port = NNetworkDefs::RADIO_MODULE_GNSS_DATA_PORT;
            broadcastData.size = sizeof(NTypes::GnssBroadcastData);
            memcpy(broadcastData.data, &coordinates, sizeof(NGnssUtils::GnssCoordinates));
            reinterpret_cast<NTypes::GnssBroadcastData*>(broadcastData.data)->updated = 1;
            loraTransmitPort.Send(broadcastData);
        }

        gnssUpdated = 0;
    }
}
