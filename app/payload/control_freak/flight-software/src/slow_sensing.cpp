#include "data.h"
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
int8_t orientation[3] = {0};
uint8_t degC = 33;
uint8_t voltage = 79;
uint8_t current = 81;

uint8_t status = 0xf0;

} // namespace LockedData

K_TIMER_DEFINE(slow_timer, NULL, NULL);

int slow_sensing_thread(void *v_fc, void *, void *) {
    FreakFlightController *fc = static_cast<FreakFlightController *>(v_fc);

    // IF LOCKED, RETURN
    bool has_marked_circle = false;
    SuperSlowPacket *slow_packet_buffer = nullptr;

    k_timer_start(&slow_timer, K_SECONDS(1), K_SECONDS(1));
    while (true) {
        mark_storage_loop_end_if_not_yet(fc, has_marked_circle);
        // LOG_INF("Allocing slab");
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
            current.Timestamp = k_uptime_get();
            ret = encode_packed_gps(current);
            if (ret != 0) {
                LOG_WRN("Invalid GPS for this slow packet: %d", ret);
            }
            if (k_mutex_lock(&i2c_data_mutex, K_MSEC(2)) == 0) {
                // current.Orientation = orientation;
                current.TempC = LockedData::degC;
                current.Current = LockedData::current;
                current.Battery_voltage = LockedData::voltage;
                current.FlipStatus = LockedData::status;
                k_mutex_unlock(&i2c_data_mutex);
            } else {
                LOG_WRN("Invalid i2c data for this slow packet");
            }
            // LOG_INF("SLow Packet %d/8", packet_index);
        }
        ret = gfs_submit_slab(superslow_storage, slow_packet_buffer, K_FOREVER);
    }
}