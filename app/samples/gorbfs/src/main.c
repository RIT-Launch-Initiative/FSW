// Zephyr Includes
#include <gorbfs.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gfs_test, CONFIG_GFS_TEST_LOG_LEVEL);

const struct device* gfs1 = DEVICE_DT_GET(DT_NODELABEL(gfs1));
const struct device* gfs2 = DEVICE_DT_GET(DT_NODELABEL(gfs2));

struct msg1 {
    // terribly ineffecient packet
    uint64_t timestamp;

    double pressure;
    double temperature;
    double co2_concentration;

    double latitude;
    double longitude;
    double altitude;
    double heading;

    // ran out of ideas
    uint8_t buffer[192];
};
BUILD_ASSERT(sizeof(struct msg1) == 256, "WANT THIS EQUAL SO WE CAN JUST CAST SLAB INTO THIS");

struct msg2item {
    uint32_t timestamp;
    uint32_t reading;
};
struct msg2 {
    struct msg2item items[32];
};
BUILD_ASSERT(sizeof(struct msg2) == 256, "WANT THIS EQUAL SO WE CAN JUST CAST SLAB INTO THIS");

bool keep_generating = true;

void worker1_fn(void*, void*, void*) {
    struct msg1* msg = NULL;
    while (!gfs_partition_saturated(gfs1)) {
        int ret = gfs_alloc_slab(gfs1, (void**) &msg, K_MSEC(1000));
        if (ret == -ENOBUFS) {
            // full, not generating any more
            break;
        } else if (ret != 0) {
            LOG_ERR("Failed to allocate slab (%d). Trying again in a second", ret);
            k_msleep(1000);
            continue;
        }

        msg->timestamp = k_uptime_get();
        if (msg->timestamp > 2000 && !gfs_circle_point_set(gfs1)) {
            // after 5 seconds, saturate rather than wrapping
            // pretend way of saying we detected boost
            gfs_signal_end_of_circle(gfs1);
        }
        msg->pressure = 103.25;
        msg->temperature = 95;
        msg->co2_concentration = 10;

        msg->latitude = 43.08630343929397;
        msg->longitude = -77.66797463569323;
        msg->altitude = 1000.4;
        msg->heading = 90.0;

        for (uint8_t i = 0; i < sizeof(msg->buffer); i++) {
            msg->buffer[i] = i;
        }
        ret = gfs_submit_slab(gfs1, msg, K_MSEC(10));
        // not needed if you submit slab with timeout of K_FOREVER
        if (ret != 0) {
            gfs_free_slab(gfs1, msg);
            LOG_WRN("Dropping slab to %s bc it could not be submitted (%d)", gfs1->name, ret);
        }
        k_msleep(1);
    }
}

K_THREAD_DEFINE(worker2, 256, worker1_fn, NULL, NULL, NULL, 1, 0, 1000);

void fill_packet2_item(struct msg2item *item){
    item->timestamp = k_uptime_get_32();
    item->reading = k_uptime_get_32() % 1000;
}

void worker2_fn(void*, void*, void*) {
    struct msg2* msg = NULL;
    while (!gfs_partition_saturated(gfs2)) {
        // allocate page for working
        int ret = gfs_alloc_slab(gfs2, (void**) &msg, K_MSEC(1000));
        if (ret == -ENOBUFS) {
            // full, not generating any more
            break;
        } else if (ret != 0) {
            printk("Failed to allocate slab (%d). Trying again in a second\n", ret);
            k_msleep(1000);
            continue;
        }

        // Actually fill with data
        for (size_t i = 0; i < sizeof(struct msg2)/sizeof(struct msg2item); i++){
            fill_packet2_item(&msg->items[i]);
            k_msleep(1);
        }

        // Submit data to flash
        ret = gfs_submit_slab(gfs2, msg, K_MSEC(10));
        // not needed if you submit slab with timeout of K_FOREVER
        if (ret != 0) {
            gfs_free_slab(gfs2, msg);
            LOG_WRN("Dropping slab to %s bc it could not be submitted (%d)", gfs2->name, ret);
        }
    }
}

K_THREAD_DEFINE(worker1, 256, worker2_fn, NULL, NULL, NULL, 1, 0, 1000);

int main() {
    struct k_poll_event events[2];
    gfs_poll_item_init(gfs1, &events[0]);
    gfs_poll_item_init(gfs2, &events[1]);
    // Just want to saturate for this one
    gfs_signal_end_of_circle(gfs2);

    while (!gfs_partition_saturated(gfs1) || !gfs_partition_saturated(gfs2)){
        int rc = k_poll(events, ARRAY_SIZE(events), K_FOREVER);
        if (rc != 0) {
            LOG_WRN("timeout waiting on k_poll");
            k_msleep(1000);
            continue;
        }
        if (GFS_POLL_ITEM_AVAILABLE(events[0]) && !gfs_partition_saturated(gfs1)) {
            rc = gfs_handle_poll_item(gfs1, &events[0]);
            if (rc == -ENOSPC) {
                LOG_INF("Saturated gfs partition '%s'\n", gfs1->name);
            } else if (rc != 0) {
                LOG_ERR("Failed to write to partition\n");
            }
        }
        if (GFS_POLL_ITEM_AVAILABLE(events[1]) && !gfs_partition_saturated(gfs2)) {
            rc = gfs_handle_poll_item(gfs2, &events[1]);
            if (rc == -ENOSPC) {
                LOG_INF("Saturated gfs partition '%s'\n", gfs2->name);
            } else if (rc != 0) {
                LOG_ERR("Failed to write to partition %d\n", rc);
            }
        }
    }

    // read data from gfs2
    static uint8_t buf[256];
    int ret = gfs_read_block(gfs2, 0, buf);
    if (ret != 0){
        LOG_ERR("Failed to read block from gfs2: %d", ret);
        return ret;
    }
    struct msg2 *msgview = (struct msg2 *)(&buf[0]);
    for (size_t i = 0; i < sizeof(msgview->items)/sizeof(msgview->items[0]); i++){
        struct msg2item *item = &msgview->items[i];
        LOG_INF("Time: %ld, Value: %ld", item->timestamp, item->reading);
    }
    return 0;
}
