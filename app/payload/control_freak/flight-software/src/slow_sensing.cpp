#include "common.h"
#include "flight.h"
#include "gorbfs.h"
#include "gps.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(slow_sense);

static const struct device *superslow_storage = DEVICE_DT_GET(DT_NODE_BY_FIXED_PARTITION_LABEL(superslow_storage));

void mark_storage_loop_end_if_not_yet(FreakFlightController *freak_controller, bool &has_marked_circle) {
    if (!has_marked_circle && freak_controller->HasEventOccurred(Events::Boost)) {
        gfs_signal_end_of_circle(superslow_storage);
        has_marked_circle = true;
    }
}

K_MUTEX_DEFINE(i2c_data_mutex);
namespace LockedData {
small_orientation orientation = {0};
uint8_t degC = 33;
uint8_t voltage = 79;
uint8_t current = 81;

uint8_t flight_state = (uint8_t) FlightState::NotSet;
uint8_t flip_state = 0xf0;
} // namespace LockedData

int submit_slowdata(const small_orientation &normed, const float &tempC, const float &current, const float &voltage,
                    FlipState fs, FlightState flight_state) {
    if (k_mutex_lock(&i2c_data_mutex, K_NO_WAIT)) {
        LOG_WRN("Couldn't submit to slow data");
        return -1;
    }
    LockedData::orientation = normed;
    LockedData::degC = tempC;
    LockedData::flip_state = fs;
    LockedData::flight_state = (uint8_t) flight_state;

    static constexpr float voltage_base = 6.75; // 6.75 to 9.25
    static constexpr float voltage_scale = 100; // 2.5 volt range to 250 volt range
    LockedData::voltage = (voltage - voltage_base) * voltage_scale;
    LockedData::current = current;

    return k_mutex_unlock(&i2c_data_mutex);
}

K_TIMER_DEFINE(slow_timer, NULL, NULL);

int slow_sensing_thread(void *v_fc, void *, void *) {
    if (is_data_locked()) {
        return -1;
    }
    FreakFlightController *fc = static_cast<FreakFlightController *>(v_fc);

    // IF LOCKED, RETURN
    bool has_marked_circle = false;
    SuperSlowPacket *slow_packet_buffer = nullptr;

    k_timer_start(&slow_timer, K_SECONDS(1), K_SECONDS(1));
    while (true) {
        mark_storage_loop_end_if_not_yet(fc, has_marked_circle);

        int ret = gfs_alloc_slab(superslow_storage, (void **) &slow_packet_buffer, K_MSEC(20));
        if (ret != 0) {
            LOG_WRN("Bad alloc of superslow buffer, will try again soon");
            // dont hog cpu, youre not all that
            k_msleep(500);
            continue;
        }

        for (int packet_index = 0; packet_index < SLOW_DATA_PER_PACKET; packet_index++) {
            k_timer_status_sync(&slow_timer);
            NTypes::SlowInfo &current = (*slow_packet_buffer)[packet_index];
            ret = encode_packed_gps_and_time(current);
            if (ret != 0) {
                LOG_WRN("Invalid GPS for this slow packet: %d", ret);
            }
            if (k_mutex_lock(&i2c_data_mutex, K_MSEC(2)) == 0) {
                // current.Orientation = orientation;
                current.TempC = LockedData::degC;
                current.Current = LockedData::current;
                current.Orientation[0] = LockedData::orientation.x;
                current.Orientation[1] = LockedData::orientation.y;
                current.Orientation[2] = LockedData::orientation.z;
                current.Battery_voltage = LockedData::voltage;
                current.FlipStatus = LockedData::flip_state;
                k_mutex_unlock(&i2c_data_mutex);
            } else {
                LOG_WRN("Invalid i2c data for this slow packet");
            }
        }
        ret = gfs_submit_slab(superslow_storage, slow_packet_buffer, K_FOREVER);
    }
}