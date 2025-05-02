#include "buzzer.h"
#include "data.h"
#include "f_core/os/c_datalogger.h"
#include "flight.h"
#include "gorbfs.h"

#include <zephyr/drivers/gnss.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_FREAK_LOG_LEVEL);

#include <zephyr/kernel.h>

static FreakFlightController freak_controller{sourceNames, eventNames, timerEvents, decisionFuncs, NULL};

// Total 4 Seconds
// .5  seconds extra (preboost + during detection)
// 1.6 seconds burn
// 0.9 seconds extra into boost
// 1 extra second to account for uc time not being real time

constexpr size_t calc_bytes(size_t seconds, size_t per_second, size_t packet_size){
    return seconds*per_second*packet_size;
}

constexpr size_t num_seconds = 4;
constexpr size_t packets_per_second = 100;
constexpr size_t packet_size = sizeof(SuperFastPacket);
constexpr size_t num_bytes = num_seconds * packets_per_second * packet_size;
static_assert(num_bytes % 256 == 0, "Man it just should");
constexpr size_t w25_page_size = 256;
constexpr size_t num_pages = num_bytes / w25_page_size;
constexpr size_t num_kilobytes = num_bytes / 1024;

struct SuperNormalPacket{
    uint64_t timestamp;
    float press;
    float temp;
    acc_data acc[2];
    gyro_data g[2];
};

constexpr size_t normal_packet_size = sizeof(SuperNormalPacket);
constexpr size_t packets_per_sec_coast = 100;
constexpr size_t num_seconds_coast = 400;
constexpr size_t coast_bytes = num_seconds_coast * packets_per_sec_coast * normal_packet_size;
constexpr size_t num_kilobytes_coast = coast_bytes / 1024;
// fast packet size = 256

struct shuntdata{
    float volt;
    float current;
};

struct SlowFlightPacket{
    uint64_t timestamp;
    shuntdata total;
    shuntdata pump;

    navigation_data nav_data;
    gnss_time timesync_time;

};

constexpr size_t slowflight_bytes = calc_bytes(400, 1, sizeof(SlowFlightPacket));
constexpr size_t slowflight_kb = slowflight_bytes / 1024;

struct SuperSlowPacket{
    uint64_t timestamp;
    shuntdata total;
    shuntdata pump;

    navigation_data nav_data;

    gnss_time timesync_time;
    float clock_skew;
    acc_data wheredyacomefromwheredyagowheredyoucomefrom;
    uint8_t radio_temp;
    
};

constexpr size_t slow_packet_size = sizeof(SuperSlowPacket);
constexpr size_t num_seconds_reco = 60*60 * 12;
constexpr size_t reco_bytes = num_seconds_reco * slow_packet_size;
constexpr size_t reco_kb = reco_bytes / 1024;

K_THREAD_DEFINE(storage, CONFIG_STORAGE_THREAD_STACK_SIZE, storage_thread_entry, (void *) &freak_controller, NULL, NULL,
                CONFIG_STORAGE_THREAD_PRIORITY, 0, 0);

K_TIMER_DEFINE(thingytimer, NULL, NULL);
int main() {
    buzzer_tell(BuzzCommand::BatteryWarning);
    k_msleep(12000);

    buzzer_tell(BuzzCommand::Silent);
    printk("Finished!\n");
    k_timer_start(&thingytimer, K_USEC(100), K_USEC(100));
    int64_t up = k_uptime_get();
    int count = 10000;
    // for (int i = 0; i < count; i++) {
    // k_timer_status_sync(&thingytimer);
    // SuperFastPacket *slab_ptr = NULL;
    // int ret = gfs_alloc_slab(&slab_ptr, K_FOREVER);
    // if (ret != 0) {
    // LOG_WRN("Non zero exit when allocing slab: %d", ret);
    // continue;
    // }
    // LOG_INF("tP = %p", slab_ptr);
    // LOG_INF("tI = %d", i);
    // slab_ptr->timestamp = 0xdeadbeef11111111; //(uint64_t) i;
    // slab_ptr->temp = 100.0;
    // slab_ptr->pressure = 2.4;
    // for (int j = 0; j < 10; j++) {
    // slab_ptr->adat[j] = {1, 2, 3};
    // slab_ptr->gdat[j] = {1, 2, 3};
    // }
    //
    // ret = gfs_submit_slab(slab_ptr, K_FOREVER);
    // if (ret != 0) {
    // LOG_WRN("Non zero exit when subitting slab: %d", ret);
    // }
    // }
    int64_t elapsed = k_uptime_get() - up;
    LOG_INF("Finished %d in %lld ms", count, elapsed);
    for (int i = 0; i < 10; i++) {
        SuperFastPacket pac = {0};
        int ret = gfs_read_block(i, &pac);
        if (ret < 0) {
            LOG_WRN("Fasiled to read block: %d", ret);
        }
        LOG_INF("AT I = %d, ts = %lld, temp = %.2f", i, pac.timestamp, pac.pressure);
    }

    while (true) {
        printk("Finished!2\n");
        k_msleep(5000);
    }
    return 0;
}