// Zephyr Includes
#include <gorbfs.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

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

struct msg2item{
    uint32_t timestamp;
    uint32_t reading;
};
struct msg2{
    struct msg2item items[32];
};
BUILD_ASSERT(sizeof(struct msg2) == 256, "WANT THIS EQUAL SO WE CAN JUST CAST SLAB INTO THIS");


bool keep_generating = true;

void worker1_fn(void*, void*, void*) {
    struct msg1* msg = NULL;
    while (keep_generating) {
        int ret = gfs_alloc_slab(gfs1, (void**) &msg, K_MSEC(1000));
        if (ret == -ENOBUFS){
            // full, not generating any more
            break;
        } else if (ret != 0) {
            printk("Failed to allocate slab (%d). Trying again in a second\n", ret);
            k_msleep(1000);
            continue;
        }
        // printk("Slot\n");
        msg->timestamp = k_uptime_get();
        if (msg->timestamp > 2000 && !gfs_circle_point_set(gfs1)) {
            // after 5 seconds, saturate rather than wrapping
            gfs_signal_end_of_circle(gfs1);
            printk("End of circle\n");
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
        if (ret != 0){
            gfs_free_slab(gfs1, msg);
            printk("asdsadasdsa\n");
        }
        k_msleep(1);
    }
    printk("Its all over, not generating and sending anything more\n");
}

K_THREAD_DEFINE(worker1, 256, worker1_fn, NULL, NULL, NULL, 1, 0, 1000);

int main() {
    struct k_poll_event events[1];
    gfs_poll_item_init(gfs1, &events[0]);
    int i = 0;
    while (true) {
        int rc = k_poll(events, ARRAY_SIZE(events), K_MSEC(1000));
        if (rc != 0) {
            printk("timeout waiting on k_poll");
            k_msleep(1000);
            continue;
        }
        if (GFS_POLL_ITEM_AVAILABLE(events[0])){
            i++;
            // printk("item %d\n", i);
            rc = gfs_handle_poll_item(gfs1, &events[0]);
            if (rc == -ENOSPC){
                // saturated, can quit now
                printk("Saturated gfs partition '%s'\n", gfs1->name);
                break;
            } else if (rc != 0){
                printk("asdasdsadasdada\n");
            }
        }
    }

    return 0;
}
