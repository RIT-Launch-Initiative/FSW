#include "data.h"
#include "f_core/os/c_datalogger.h"
#include "flight.h"
#include "gorbfs.h"

#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/reboot.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_FREAK_LOG_LEVEL);

#include <zephyr/kernel.h>

static FreakFlightController freak_controller{sourceNames, eventNames, timerEvents, decisionFuncs, NULL};

static const struct device *superfast_storage = DEVICE_DT_GET(DT_NODE_BY_FIXED_PARTITION_LABEL(superfast_storage));



K_THREAD_DEFINE(storage, CONFIG_STORAGE_THREAD_STACK_SIZE, storage_thread_entry, (void *) &freak_controller,
                (void *) &superfast_storage, NULL, CONFIG_STORAGE_THREAD_PRIORITY, 0, 0);

int main() {
    
    if (!device_is_ready(superfast_storage)) {
        LOG_ERR("Storage not ready");
    }

    return 0;
}
