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

    // disable servos
    // enable lora
    // GPS configure

    //Ground, Boost, Coast, Flight
    ret = boost_and_flight_sensing(imu_dev, barom_dev, &freak_controller);
    LOG_INF("On the ground now");

    return 0;
}

int cmd_unlock(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "Unlocking boost data");
    unlock_boostdata();
    shell_print(shell, "Success, rebooting....\n\n\n");
    sys_reboot(SYS_REBOOT_COLD);
}

int cmd_stop(const struct shell *shell, size_t argc, char **argv);

int cmd_boost(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "Boost");
    freak_controller.SubmitEvent(Sources::LSM6DSL, Events::Boost);
    return 0;
}

int cmd_readfast(const struct shell *shell, size_t argc, char **argv) {
    static uint8_t buf[256] = {0};
    for (int i = 0; i < gfs_total_blocks(superfast_storage); i++) {
        gfs_read_block(superfast_storage, i, buf);
        shell_hexdump(shell, buf, 256);
    }
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(test_subcmds, SHELL_CMD(stop, NULL, "Stop Test", cmd_stop),
                               SHELL_CMD(dumpgorb, NULL, "Dump Gorb partition", cmd_readfast),
                               SHELL_CMD(unlocl, NULL, "Unlock flight data partition", cmd_unlock),
                               SHELL_CMD(boost, NULL, "fake boost detect", cmd_boost), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(test, &test_subcmds, "Test Commands", NULL);
