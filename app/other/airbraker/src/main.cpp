#include "5v_ctrl.h"
#include "boost.h"
#include "common.h"
#include "f_core/os/c_datalogger.h"
#include "fast_sensing.h"
#include "flight.h"
#include "gorbfs.h"
#include "storage.h"

#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/base64.h>
#include <zephyr/sys/reboot.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_AIRBRAKE_LOG_LEVEL);

#include <zephyr/kernel.h>
constexpr float BATTERY_WARNING_THRESH = 7.9;
float startup_voltage = 0;

// extern struct k_msgq flightlog_msgq;
// void handler(FreakFlightController::EventNotification noti) { k_msgq_put(&flightlog_msgq, &noti, K_MSEC(1)); }

// FreakFlightController freak_controller{sourceNames, eventNames, timerEvents, decisionFuncs, handler};
// static const struct device *superfast_storage = DEVICE_DT_GET(DT_NODE_BY_FIXED_PARTITION_LABEL(superfast_storage));

// Partitions parts{
// .superfast_dev = superfast_storage,
// };

// K_THREAD_DEFINE(storage, CONFIG_STORAGE_THREAD_STACK_SIZE, storage_thread_entry, (void *) &freak_controller,
// (void *) &parts, NULL, CONFIG_STORAGE_THREAD_PRIORITY, 0, 0);

int main() {
    int ret = 0;
    if (is_data_locked()) {
        // buzzer_tell(BuzzCommand::DataLocked);
        return -1;
    }
    ret = five_volt_rail_init();
    if (ret != 0) {
        LOG_ERR("Failed to init 5v rail control");
        // buzzer_tell(BuzzCommand::SensorTroubles);
    }

    const struct device *imu_dev = DEVICE_DT_GET(DT_ALIAS(imu));
    const struct device *barom_dev = DEVICE_DT_GET(DT_ALIAS(barom));

    const struct device *ina_servo = DEVICE_DT_GET(DT_ALIAS(inaservo));
    const struct device *ina_pump = DEVICE_DT_GET(DT_ALIAS(inapump));

    if (!device_is_ready(imu_dev) || !device_is_ready(barom_dev)) {
        LOG_ERR("Sensor devices not ready");
        // buzzer_tell(BuzzCommand::SensorTroubles);
        return -1;
    }
    if (!device_is_ready(ina_servo) || !device_is_ready(ina_pump)) {
        LOG_ERR("Sensor devices not ready");
        // buzzer_tell(BuzzCommand::SensorTroubles);
        return -1;
    }
    float current = 0;

    ret = read_ina(ina_servo, startup_voltage, current);
    if (ret != 0) {
        LOG_ERR("Couldn't read battery");
    }
    if (startup_voltage < BATTERY_WARNING_THRESH) {
        // buzzer_tell(BuzzCommand::BatteryWarning);
    }

    // if (!device_is_ready(superfast_storage)) {
    // LOG_ERR("Storage not ready");
    // buzzer_tell(BuzzCommand::SensorTroubles);
    // }

    // buzzer_tell(BuzzCommand::AllGood);

    //Ground, Boost, Coast, Flight
    // ret = boost_and_flight_sensing(superfast_storage, imu_dev, barom_dev, ina_servo, &freak_controller);
    LOG_INF("On the ground now");

    return 0;
}

int cmd_unlock(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "Unlocking boost data");
    unlock_boostdata();
    // buzzer_tell(BuzzCommand::AllGood);
    return 0;
}

int cmd_lock(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "Locking boost data");
    lock_boostdata();
    return 0;
}

int cmd_stop(const struct shell *shell, size_t argc, char **argv);

int cmd_shutup(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "Acknowledging that you dont care that data is locked");
    // buzzer_tell(BuzzCommand::AllGood);
    return 0;
}

extern bool overriding_boost;
int cmd_boost(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "Boost");
    overriding_boost = true;
    return 0;
}
int cmd_preflight(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "Preflight Checklist");
    shell_print(shell, "Cameras on");
    shell_print(shell, "Startup Voltage: %.3f", (double) startup_voltage);
    if (startup_voltage < BATTERY_WARNING_THRESH) {
        LOG_ERR("BATTERY LOW");
        // buzzer_tell(BuzzCommand::BatteryWarning);
    }
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
    return 0;
    // return cmd_read(superfast_storage, shell, argc, argv);
}
SHELL_STATIC_SUBCMD_SET_CREATE(freak_subcmds, SHELL_CMD(stop, NULL, "Stop Test", cmd_stop),
                               SHELL_CMD(unlock, NULL, "Unlock flight data partition", cmd_unlock),
                               SHELL_CMD(lock, NULL, "Lock flight data partition", cmd_lock),
                               SHELL_CMD(boost, NULL, "fake boost detect", cmd_boost),
                               SHELL_CMD(shut, NULL, "stop yapping", cmd_shutup), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(freak, &freak_subcmds, "Test Commands", NULL);

SHELL_STATIC_SUBCMD_SET_CREATE(gorb_subcmds, SHELL_CMD(dumpfast, NULL, "Dump fast partition", cmd_readfast),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(gorb, &gorb_subcmds, "Gorbfs Commands", NULL);