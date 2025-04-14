#include "c_gnss_tenant.h"
#include "c_radio_module.h"

#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CGnssTenant);

static NTypes::GnssData gnssData{0};
static uint8_t gnssUpdated = 0;

static void gnssCallback(const device *, const gnss_data *data) {
    static constexpr float zephyrGnssScale = 1e9f;
    static const device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
    gnssData.Coordinates.Latitude = static_cast<float>(data->nav_data.latitude) / zephyrGnssScale;
    gnssData.Coordinates.Longitude = static_cast<float>(data->nav_data.longitude) / zephyrGnssScale;
    gnssData.Coordinates.Altitude = static_cast<float>(data->nav_data.altitude) / 1000.0f;

    gnssData.Info.FixQuality = data->info.fix_quality;
    gnssData.Info.FixStatus = data->info.fix_status;
    gnssData.Info.HorizontalDilution = data->info.hdop;
    gnssData.Info.SatelliteCount = data->info.satellites_cnt;

    gnssData.Time.Hour = data->utc.hour;
    gnssData.Time.Minute = data->utc.minute;
    gnssData.Time.Millisecond = data->utc.millisecond;
    gnssData.Time.DayOfMonth = data->utc.month_day;
    gnssData.Time.Month = data->utc.month;
    gnssData.Time.YearOfCentury = data->utc.century_year;

    gnssUpdated = 1;

    LOG_DBG("Latitude: %f, Longitude: %f, Altitude: %f",
        static_cast<double>(gnssData.Coordinates.Latitude),
        static_cast<double>(gnssData.Coordinates.Longitude),
        static_cast<double>(gnssData.Coordinates.Altitude));

    // Set the rtc time
    rtc_time lastUpdated = {
        .tm_sec = data->utc.millisecond / 1000,
        .tm_min = data->utc.minute,
        .tm_hour = data->utc.hour,
        .tm_mday = data->utc.month_day,
        .tm_mon = data->utc.month,
        .tm_year = data->utc.century_year - 1900,
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
    NTypes::LoRaBroadcastData broadcastData{0};
    NTypes::GnssData logData{0};

    if (gnssUpdated) {
        memcpy(&logData, &gnssData, sizeof(NTypes::GnssData));
        dataLoggingPort.Send(logData);

        if (transmitTimer.IsExpired()) {
            NTypes::GnssBroadcastPacket broadcastPacket {
                .Coordinates = gnssData.Coordinates,
                .SatelliteCount = gnssData.Info.SatelliteCount,
                .FixStatus = gnssData.Info.FixStatus,
                .FixQuality = gnssData.Info.FixQuality
            };

            broadcastData.Port = NNetworkDefs::RADIO_MODULE_GNSS_DATA_PORT;
            broadcastData.Size = sizeof(NTypes::GnssBroadcastPacket);
            memcpy(broadcastData.Payload, &broadcastPacket, sizeof(NTypes::GnssBroadcastPacket));
            loraTransmitPort.Send(broadcastData);
        }

        gnssUpdated = 0;
    }
}
