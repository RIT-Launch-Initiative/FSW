#include "boost.h"
#include "buzzer.h"
#include "cycle_counter.hpp"
#include "data.h"
#include "f_core/os/c_datalogger.h"
#include "fast_sensing.h"
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

// Dummy implementation, will use actual flash at some point

bool boostdata_locked = false;
void unlock_boostdata() { boostdata_locked = false; }
void lock_boostdata() { boostdata_locked = true; }
bool is_boostdata_locked() { return boostdata_locked; }

#define IMU_NODE DT_ALIAS(imu)
static const struct device *imu_dev = DEVICE_DT_GET(IMU_NODE);

#define BAROM_NODE DT_ALIAS(barom)
static const struct device *barom_dev = DEVICE_DT_GET(BAROM_NODE);

static const struct device *superfast_storage = DEVICE_DT_GET(DT_NODE_BY_FIXED_PARTITION_LABEL(superfast_storage));

K_THREAD_DEFINE(storage, CONFIG_STORAGE_THREAD_STACK_SIZE, storage_thread_entry, (void *) &freak_controller,
                (void *) &superfast_storage, NULL, CONFIG_STORAGE_THREAD_PRIORITY, 0, 0);

int main() {
    int ret = 0;
    buzzer_tell(BuzzCommand::Silent);

    if (!device_is_ready(imu_dev) || !device_is_ready(barom_dev)) {
        LOG_ERR("Devices not ready");
        return -1;
    }
    if (!device_is_ready(superfast_storage)) {
        LOG_ERR("Storage not ready");
    }

    ret = flight_sensing(imu_dev, barom_dev, &freak_controller);
    return 0;
}

int cmd_unlock(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "Unlocking boost data");
    unlock_boostdata();
    shell_print(shell, "Success, rebooting....\n\n\n");
    sys_reboot(SYS_REBOOT_COLD);
}

int cmd_stop(const struct shell *shell, size_t argc, char **argv);

int cmd_readall(const struct shell *shell, size_t argc, char **argv) {
    // SuperFastPacket pac;
    // shell_fprintf_normal(shell, "ts, temp, press, ax, ay, az, gx, gy, gz\n");
    //
    // for (int i = 0; i < gfs_total_blocks(); i++) {
    // int ret = gfs_read_block(i, &pac);
    // if (ret != 0) {
    // LOG_WRN("Couldnt read page # %d: %d", i, ret);
    // }
    // for (int j = 0; j < 10; j++) {
    // if (j == 0) {
    // shell_fprintf_normal(shell, "%lld, %f, %f,", pac.timestamp, (double) pac.temp, (double) pac.pressure);
    // } else {
    // shell_fprintf_normal(shell, "NaN, NaN, NaN,");
    // }
    // shell_fprintf_normal(shell, "%f, %f, %f, %f, %f, %f\n", (double) pac.adat[j].ax, (double) pac.adat[j].ay,
    //  (double) pac.adat[j].az, (double) pac.gdat[j].gx, (double) pac.gdat[j].gy,
    //  (double) pac.gdat[j].gz);
    // }
    // k_msleep(10);
    // }
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(test_subcmds, SHELL_CMD(stop, NULL, "Stop Test", cmd_stop),
                               SHELL_CMD(dumpgorb, NULL, "Dump Gorb partition", cmd_readall),
                               SHELL_CMD(unlocl, NULL, "Unlock flight data partition", cmd_unlock),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(test, &test_subcmds, "Test Commands", NULL);
