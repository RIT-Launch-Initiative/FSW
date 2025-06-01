#include "5v_ctrl.h"
#include "boost.h"
#include "buzzer.h"
#include "cameras.h"
#include "common.h"
#include "cycle_counter.hpp"
#include "f_core/os/c_datalogger.h"
#include "fast_sensing.h"
#include "flight.h"
#include "flipping.h"
#include "gorbfs.h"
#include "slow_sensing.h"

#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/base64.h>
#include <zephyr/sys/reboot.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_FREAK_LOG_LEVEL);

#include <zephyr/kernel.h>

FreakFlightController freak_controller{sourceNames, eventNames, timerEvents, decisionFuncs, NULL};

// Dummy implementation, will use actual flash at some point

bool boostdata_locked = false;
void unlock_boostdata() { boostdata_locked = false; }
void lock_boostdata() { boostdata_locked = true; }
bool is_boostdata_locked() { return boostdata_locked; }

// SYS_INIT(init_boostdata_locked)

static const struct device *superfast_storage = DEVICE_DT_GET(DT_NODE_BY_FIXED_PARTITION_LABEL(superfast_storage));
static const struct device *superslow_storage = DEVICE_DT_GET(DT_NODE_BY_FIXED_PARTITION_LABEL(superslow_storage));

int radio_thread(void *, void *, void *);
K_THREAD_DEFINE(radio, CONFIG_STORAGE_THREAD_STACK_SIZE, radio_thread, (void *) &freak_controller,
                (void *) &superfast_storage, NULL, CONFIG_STORAGE_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(storage, CONFIG_STORAGE_THREAD_STACK_SIZE, storage_thread_entry, (void *) &freak_controller,
                (void *) &superfast_storage, (void *) &superslow_storage, CONFIG_STORAGE_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(slow_sensing, CONFIG_SLOWDATA_THREAD_STACK_SIZE, slow_sensing_thread, (void *) &freak_controller, NULL,
                NULL, CONFIG_SLOWDATA_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(cameras, 512, camera_thread_entry, (void *) &freak_controller, NULL, NULL,
                CONFIG_SLOWDATA_THREAD_PRIORITY, 0, 0);

int main() {
    int ret = 0;

    const struct device *imu_dev = DEVICE_DT_GET(DT_ALIAS(imu));
    const struct device *barom_dev = DEVICE_DT_GET(DT_ALIAS(barom));

    const struct device *ina_servo = DEVICE_DT_GET(DT_ALIAS(inaservo));
    const struct device *ina_pump = DEVICE_DT_GET(DT_ALIAS(inapump));

    buzzer_tell(BuzzCommand::Silent);

    if (!device_is_ready(imu_dev) || !device_is_ready(barom_dev)) {
        LOG_ERR("Sensor devices not ready");
        return -1;
    }
    if (!device_is_ready(ina_servo) || !device_is_ready(ina_pump)) {
        LOG_ERR("Sensor devices not ready");
        return -1;
    }
    if (!device_is_ready(superfast_storage)) {
        LOG_ERR("Storage not ready");
    }

    ret = init_flip_hw();
    if (ret != 0) {
        LOG_ERR("Error initializing servo hardware");
    }
    ret = five_volt_rail_init();
    if (ret != 0) {
        LOG_ERR("Failed to init 5v rail control");
    }

    //Ground, Boost, Coast, Flight
    ret = boost_and_flight_sensing(superfast_storage, imu_dev, barom_dev, &freak_controller);
    LOG_INF("On the ground now");
    do_flipping_and_pumping(imu_dev, barom_dev, ina_servo, ina_pump);

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
int cmd_inflated(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "Inflated");
    freak_controller.SubmitEvent(Sources::InflationSubsys, Events::InitialInflation);
    return 0;
}

int hexdump(const struct shell *shell, uint8_t *buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
        shell_fprintf_normal(shell, "%02X", buf[i]);
        if (i % 32 == 31 && i != 0) {
            shell_print(shell, "");
        }
    }

    return 0;
}

// dump base64 data here before printing
#define DUMP_BUFFER_LEN (256 * 2)
uint8_t dumping_data[DUMP_BUFFER_LEN + 1];

static int b64dump(const struct shell *shell, uint8_t *buf) {
    int ret;
    size_t num_written = 0;
    ret = base64_encode(dumping_data, DUMP_BUFFER_LEN, &num_written, buf, 256);
    if (ret < 0) {
        shell_print(shell, "encoding error %d, num_written: %d of 256", ret, num_written);
        return ret;
    }
    dumping_data[DUMP_BUFFER_LEN] = 0; // null terminator
    shell_print(shell, "%s", dumping_data);
    return 0;
}

int cmd_read(const struct device *dev, const struct shell *shell, size_t argc, char **argv) {
    int blocks = gfs_total_blocks(dev);
    if (argc == 1) {

    } else if (argc == 2) {
        int num_blocks = atoi(argv[1]);
        if (num_blocks == 0) {
            shell_error(shell, "Error parsing number or 0 blocks requested");
            return -1;
        }
        if (num_blocks > blocks) {
            shell_error(shell, "Cant read %d blocks of %d", num_blocks, blocks);
            return -1;
        }
        blocks = num_blocks;

    } else {
        shell_error(shell, "I want one argument, number of blocks or none to read the whole partition");
        return -1;
    }
    // b64 dump in 256 byte lines: 500 blocks in 36.51 sec
    shell_print(shell, "************ gorb start %s ************", dev->name);
    static uint8_t buf[256] = {0};
    for (int i = 0; i < blocks; i++) {
        gfs_read_block(dev, i, buf);
        hexdump(shell, buf, 256); // TODO investigate b64 slowdown time
    }
    shell_print(shell, "************ gorb end %s ************", dev->name);
    return 0;
}

int cmd_read_compress(const struct device *dev, const struct shell *shell, size_t argc, char **argv) {
    int blocks = gfs_total_blocks(dev);
    if (argc == 1) {

    } else if (argc == 2) {
        int num_blocks = atoi(argv[1]);
        if (num_blocks == 0) {
            shell_error(shell, "Error parsing number or 0 blocks requested");
            return -1;
        }
        if (num_blocks > blocks) {
            shell_error(shell, "Cant read %d blocks of %d", num_blocks, blocks);
            return -1;
        }
        blocks = num_blocks;

    } else {
        shell_error(shell, "I want one argument, number of blocks or none to read the whole partition");
        return -1;
    }
    // b64 dump in 256 byte lines: 500 blocks in 36.51 sec
    // hexdump 500 blocks in 25.56
    shell_print(shell, "************ gorb start %s ************", dev->name);
    static uint8_t buf[256] = {0};
    for (int i = 0; i < blocks; i++) {
        gfs_read_block(dev, i, buf);
        hexdump(shell, buf, 256);
    }
    shell_print(shell, "************ gorb end %s ************", dev->name);
    return 0;
}

int cmd_readfast(const struct shell *shell, size_t argc, char **argv) {
    return cmd_read(superfast_storage, shell, argc, argv);
}
int cmd_readslow(const struct shell *shell, size_t argc, char **argv) {
    return cmd_read(superslow_storage, shell, argc, argv);
}

SHELL_STATIC_SUBCMD_SET_CREATE(test_subcmds, SHELL_CMD(stop, NULL, "Stop Test", cmd_stop),
                               SHELL_CMD(unlocl, NULL, "Unlock flight data partition", cmd_unlock),
                               SHELL_CMD(boost, NULL, "fake boost detect", cmd_boost),
                               SHELL_CMD(inflated, NULL, "fake inflated", cmd_inflated), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(test, &test_subcmds, "Test Commands", NULL);

SHELL_STATIC_SUBCMD_SET_CREATE(gorb_subcmds, SHELL_CMD(dumpfast, NULL, "Dump fast partition", cmd_readfast),
                               SHELL_CMD(dumpfast, NULL, "Dump slow partition", cmd_readslow), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(gorb, &gorb_subcmds, "Gorbfs Commands", NULL);