#include "gps.h"

#include "common.h"
#include "f_core/utils/n_gnss_utils.h"
#include "flight.h"
#include "gorbfs.h"
#include "ublox_m10.h"

#include <math.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gps_handler);

#define GPS_NODE DT_ALIAS(gnss)
static const struct device *gps_dev = DEVICE_DT_GET(GPS_NODE);

K_MUTEX_DEFINE(gps_mutex);

#define MAX_SATS 20
struct gnss_data last_data;
struct gnss_satellite last_sats[MAX_SATS];

int64_t last_fix_uptime_ticks = 0;
float last_valid_skew_factor = 1.0090; // 10090

uint32_t millis_since_start_of_day(const gnss_time &time) {
    static constexpr uint32_t millis_per_minute = 60 * 1000;
    static constexpr uint32_t millis_per_hour = 60 * millis_per_minute;
    return time.hour * millis_per_hour + time.minute * millis_per_minute + time.millisecond;
}
// Comment copied from types.yaml
// Lat/Long stored as 1/65536 degree. 1 LSB = 1.69m
// 14 bits alt (1 ft / LSB) (0-16384 in case we overshoot or fall into hell)
// 10 bits speed (1 m/s / LSB) (0-1024m/s (OR says max 800))
// 3 bits flight state (0 notready, 1 waiting, 2 boost, 3 coast, 4 ground, 5 flipping, 6 transmitting, 7 error)
// 2 bits fix status (0 None, 1 GNSS Fix, 2 DGNSS Fix, 3 Estimated Fix)
// 3 bits fix quality (see zephyr enum)
//
// 4 bits satelites (max 16 in sight at once http://csno-tarc.cn/en/gps/number)
// 27 bits millis since start of day
// 1 bit for fun
int encode_packed_gps_and_time(NTypes::SlowInfo &output) {
    if (k_mutex_lock(&gps_mutex, K_MSEC(20)) != 0) {
        return -1;
    }
    if (last_data.info.fix_status == GNSS_FIX_STATUS_NO_FIX) {
        output.Timestamp = k_uptime_ticks();
    } else {
        output.Timestamp = last_fix_uptime_ticks;
    }
    float lat = last_data.nav_data.latitude;
    float lon = last_data.nav_data.longitude;
    int32_t alt_mm = last_data.nav_data.altitude;     // mm asl
    uint32_t speed_mm_p_s = last_data.nav_data.speed; // mm / s
    uint32_t millis = millis_since_start_of_day(last_data.utc);
    gnss_fix_quality qual = last_data.info.fix_quality;
    gnss_fix_status stat = last_data.info.fix_status;
    uint8_t sats = last_data.info.satellites_cnt;

    lat = fabs(lat);
    lat -= floorf(lat);

    lon = fabs(lon);
    lon -= floorf(lon);

    output.LatFrac = lat * UINT16_MAX;
    output.LongFrac = lon * UINT16_MAX;

    uint16_t alt_feet = alt_mm * 0.00328084f;
    alt_feet &= BIT_MASK(14);

    uint16_t speed = speed_mm_p_s / 1000;
    speed &= BIT_MASK(10);

    uint8_t stat_bits = stat & BIT_MASK(2);
    uint8_t qual_bits = qual & BIT_MASK(3);

    uint32_t highside = (alt_feet << 18) | (speed << 8) | (stat_bits << 3) | qual_bits;

    uint32_t lowside = ((sats & BIT_MASK(4)) << 28) | ((millis & BIT_MASK(28)) << 1);

    output.PackedData = (((uint64_t) highside) << 32) | lowside;
    return k_mutex_unlock(&gps_mutex);
}
extern int64_t asdf_ticks;
uint32_t millis_till_timeslot_opens() {
    if (last_data.info.fix_status == GNSS_FIX_STATUS_NO_FIX) {
        return CONFIG_HORUS_TIMESLOT_SECONDS * 1000;
    }
    k_ticks_t last_tick_delta = ublox_10_last_tick_delta(gps_dev);
    int64_t last_tick_uptime_ticks = ublox_10_last_tick_uptime(gps_dev);
    LOG_INF("uptiem ticks:%lld, %lld", asdf_ticks, last_tick_uptime_ticks);
    last_tick_uptime_ticks = asdf_ticks;
    // Assume timepulse was for the last NMEA time sentence, correct later if
    // not
    uint32_t seconds_from_hour_start_of_last_timepulse = last_data.utc.minute * 60 + last_data.utc.millisecond / 1000;
    if (last_fix_uptime_ticks < last_tick_uptime_ticks) {
        // A time pulse has come in after that NMEA sentence
        // Add 1 second to compensate
        seconds_from_hour_start_of_last_timepulse++;
    }
    LOG_INF("TP since top of hour: %d", seconds_from_hour_start_of_last_timepulse);

    int32_t seconds_from_time_sync_start =
        seconds_from_hour_start_of_last_timepulse - CONFIG_HORUS_TIMESLOT_OFFSET_SECONDS;

    if (seconds_from_time_sync_start < 0) {
        // if before time slice time, pretend we're going from the previous hour
        seconds_from_time_sync_start += 60 * 60;
    }
    LOG_INF("seconds from time sync start: %d", seconds_from_time_sync_start);

    uint32_t periods_so_far = seconds_from_time_sync_start / CONFIG_HORUS_TIMESLOT_SECONDS;
    LOG_INF("%d periods so far", periods_so_far);
    uint32_t offset_into_this_period = seconds_from_time_sync_start - (CONFIG_HORUS_TIMESLOT_SECONDS * periods_so_far);
    LOG_INF("offset into this period %d", offset_into_this_period);

    uint32_t seconds_from_timepulse_to_next_slot = CONFIG_HORUS_TIMESLOT_SECONDS - offset_into_this_period;
    LOG_INF("Seconds from timepulse to next slot: %d", seconds_from_timepulse_to_next_slot);
    int64_t now_ms = k_uptime_get();
    int64_t tp_ms = k_ticks_to_ms_near64(last_tick_uptime_ticks);
    LOG_INF("Last tick at %lld  ms", tp_ms);

    int64_t uptime_ms_of_next_slot = tp_ms + seconds_from_timepulse_to_next_slot * 1000;
    LOG_INF("ms of next slot %lld  ms, now %lld", uptime_ms_of_next_slot, now_ms);

    int32_t ms_left = uptime_ms_of_next_slot - now_ms;
    if (ms_left < 0 || ms_left > (int) (CONFIG_HORUS_TIMESLOT_SECONDS * 1000)) {
        printf("ERROR calculating time sync: %d wanted < %d\n", ms_left, (int) (CONFIG_HORUS_TIMESLOT_SECONDS * 1000));
        return CONFIG_HORUS_TIMESLOT_SECONDS * 1000;
    }

    return ms_left;
}

float get_skew_smart() { return last_valid_skew_factor; }

int fill_packet(struct horus_packet_v2 *packet) {
    static constexpr float mm_per_sec_to_km_per_hour = 0.0036;

    if (k_mutex_lock(&gps_mutex, K_MSEC(20)) != 0) {
        return -1;
    }
    packet->latitude = NGnssUtils::NanodegreesToDegreesFloat(last_data.nav_data.latitude);
    packet->longitude = NGnssUtils::NanodegreesToDegreesFloat(last_data.nav_data.longitude);
    packet->altitude = last_data.nav_data.altitude / 1000;
    float speed_mm_per_sec = last_data.nav_data.speed;

    packet->hours = last_data.utc.hour;
    packet->minutes = last_data.utc.minute;
    packet->seconds = last_data.utc.millisecond / 1000;

    packet->sats = last_data.info.satellites_cnt;

    k_mutex_unlock(&gps_mutex);

    float speed_kph = speed_mm_per_sec * mm_per_sec_to_km_per_hour;
    if (speed_kph > 255) {
        speed_kph = 255;
    } else if (speed_kph < 0) {
        speed_kph = 0;
    }
    packet->speed = (uint8_t) speed_kph;

    return 0;
}

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data) {
    ARG_UNUSED(dev);
    int ret = k_mutex_lock(&gps_mutex, K_MSEC(20));
    if (ret != 0) {
        LOG_WRN("Failed to lock gps mutex: %d", ret);
        return;
    }
    last_data = *data;
    if (data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
        last_fix_uptime_ticks = k_uptime_ticks();
        // LOG_INF("Got GPS");
        // LOG_INF("Quality: %d, Status: %d, Sats: %d", data->info.fix_quality, data->info.fix_status,
        // data->info.satellites_cnt);
        //
        // LOG_INF("Lat: %lld, Long: %lld", data->nav_data.latitude, data->nav_data.longitude);
        // LOG_INF("%d/%d/%d %02d:%02d:%d", data->utc.month, data->utc.month_day, data->utc.century_year, data->utc.hour,
        // data->utc.minute, data->utc.millisecond);
    } else {
        LOG_WRN("No Fix");
    }
    k_mutex_unlock(&gps_mutex);
}
GNSS_DATA_CALLBACK_DEFINE(gps_dev, gnss_data_cb);
