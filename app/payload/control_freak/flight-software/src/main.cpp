#include "buzzer.h"
#include "data.h"
#include "f_core/os/c_datalogger.h"
#include "flight.h"
#include "gorbfs.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_FREAK_LOG_LEVEL);

#include <zephyr/kernel.h>

static FreakFlightController freak_controller{sourceNames, eventNames, timerEvents, decisionFuncs, NULL};

// Total 4 Seconds
// .5  seconds extra (preboost + during detection)
// 1.6 seconds burn
// 0.9 seconds extra into boost
// 1 extra second to account for uc time not being real time

constexpr size_t num_seconds = 4;
constexpr size_t packets_per_second = 100;
constexpr size_t packet_size = sizeof(SuperFastPacket);
constexpr size_t num_bytes = num_seconds * packets_per_second * packet_size;
static_assert(num_bytes % 256 == 0, "Man it just should");
constexpr size_t w25_page_size = 256;
constexpr size_t num_pages = num_bytes / w25_page_size;
// fast packet size = 256

K_THREAD_DEFINE(storage, CONFIG_STORAGE_THREAD_STACK_SIZE, storage_thread_entry, (void *) &freak_controller, NULL, NULL,
                CONFIG_STORAGE_THREAD_PRIORITY, 0, 0);

K_TIMER_DEFINE(thingytimer, NULL, NULL);
int main() {
    buzzer_tell(BuzzCommand::Silent);
    k_msleep(1000);
    // buzzer_tell(BuzzCommand::Silent);
    printk("Finished!\n");
    k_timer_start(&thingytimer, K_USEC(100), K_USEC(100));

    for (int i = 0; i < 100; i++) {
        k_timer_status_sync(&thingytimer);
        SuperFastPacket *slab_ptr = NULL;
        int ret = gfs_alloc_slab(&slab_ptr, K_FOREVER);
        if (ret != 0) {
            LOG_WRN("Non zero exit when allocing slab: %d", ret);
            continue;
        }
        LOG_INF("tP = %p", slab_ptr);
        LOG_INF("tI = %d", i);
        slab_ptr->timestamp = (uint64_t) i;
        ret = gfs_submit_slab(slab_ptr, K_FOREVER);
        if (ret != 0) {
            LOG_WRN("Non zero exit when subitting slab: %d", ret);
        }
    }
    while (true) {
        printk("Finished!2\n");
        k_msleep(1000);
    }
    return 0;
}