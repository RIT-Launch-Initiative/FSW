#include "boost.h"
#include "buzzer.h"
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

K_THREAD_DEFINE(storage, CONFIG_STORAGE_THREAD_STACK_SIZE, storage_thread_entry, (void *) &freak_controller, NULL, NULL,
                CONFIG_STORAGE_THREAD_PRIORITY, 0, 0);

#define IMU_NODE DT_ALIAS(imu)
static const struct device *imu_dev = DEVICE_DT_GET(IMU_NODE);

#define BAROM_NODE DT_ALIAS(barom)
static const struct device *barom_dev = DEVICE_DT_GET(BAROM_NODE);

K_TIMER_DEFINE(imutimer, NULL, NULL);

bool DONT_STOP = true;

int overcounter = 0;

class CycleCounter {
  public:
    CycleCounter(int64_t &toAdd) : toAdd(toAdd) { start_cyc = k_cycle_get_64(); }
    ~CycleCounter() { toAdd += k_cycle_get_64() - start_cyc; }

    CycleCounter(CycleCounter &&) = delete;
    CycleCounter(const CycleCounter &) = delete;
    CycleCounter &operator=(CycleCounter &&) = delete;
    CycleCounter &operator=(const CycleCounter &) = delete;

  private:
    int64_t &toAdd;
    int64_t start_cyc;
};

int main() {
    buzzer_tell(BuzzCommand::Silent);

    if (!device_is_ready(imu_dev) || !device_is_ready(barom_dev)) {
        LOG_ERR("Devices not ready");
        return -1;
    }

    struct sensor_value sampling = {0};
    sensor_value_from_float(&sampling, 1666);
    int ret = sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling);
    if (ret < 0) {
        LOG_ERR("Couldnt set sampling\n");
    }
    ret = sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling);
    if (ret < 0) {
        LOG_ERR("Couldnt set sampling\n");
    }

    int64_t start = k_uptime_get();

    int perPacket = 10;
    int frame = 0;
    SuperFastPacket *packet = NULL;

    k_timer_start(&imutimer, K_USEC(100), K_USEC(100));
    int packets_sent = 0;

    int64_t cyc_read_imu = 0;
    int64_t cyc_read_barom = 0;
    int64_t cyc_slabs = 0;
    int64_t cyc_syncing = 0;
    int64_t cyc_calc_boost = 0;
    int64_t total_start = k_cycle_get_64();
    while (DONT_STOP) {
        {
            CycleCounter _(cyc_syncing);
            // k_timer_status_sync(&imutimer);
        }

        if (frame == 0) {
            int ret = 0;
            {
                CycleCounter _(cyc_slabs);
                ret = gfs_alloc_slab(&packet, K_FOREVER);
            }
            int64_t us = k_ticks_to_us_near64(k_uptime_ticks());
            packet->timestamp = us;

            {
                CycleCounter _(cyc_read_barom);
                // read barometer
                ret = sensor_sample_fetch(barom_dev);
                if (ret != 0) {
                    LOG_WRN("Couldnt read barometer: %d", ret);
                }
                struct sensor_value val;
                ret = sensor_channel_get(barom_dev, SENSOR_CHAN_PRESS, &val);
                if (ret != 0) {
                    LOG_WRN("Couldnt get pres from barometer: %d", ret);
                }
                packet->pressure = sensor_value_to_float(&val);

                ret = sensor_channel_get(barom_dev, SENSOR_CHAN_AMBIENT_TEMP, &val);
                if (ret != 0) {
                    LOG_WRN("Couldnt get temp from barometer: %d", ret);
                }
                packet->temp = sensor_value_to_float(&val);
            }
        }
        // read imu
        {
            CycleCounter _(cyc_read_imu);
            sensor_sample_fetch(imu_dev);
            struct sensor_value vals[3];
            ret = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, vals);
            if (ret != 0) {
                LOG_WRN("Couldnt get axyz");
            }
            packet->adat[frame].ax = sensor_value_to_float(&vals[0]);
            packet->adat[frame].ay = sensor_value_to_float(&vals[1]);
            packet->adat[frame].az = sensor_value_to_float(&vals[2]);

            ret = sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, vals);
            if (ret != 0) {
                LOG_WRN("Couldnt get gxyz");
            }
            packet->gdat[frame].gx = sensor_value_to_float(&vals[0]);
            packet->gdat[frame].gy = sensor_value_to_float(&vals[1]);
            packet->gdat[frame].gz = sensor_value_to_float(&vals[2]);
        }

        {
            CycleCounter _(cyc_calc_boost);
            bool is_boosted = feed_boost_acc(k_uptime_get(), &packet->adat[frame].ax);
            if (is_boosted) {
                break;
            }
        }
        frame++;
        if (frame == perPacket) {

            // send packet
            packets_sent++;
            {
                CycleCounter _(cyc_slabs);
                ret = gfs_submit_slab(packet, K_FOREVER);
            }

            if (ret != 0) {
                LOG_WRN("Couldnt submit slab: %d", ret);
            }
            frame = 0;
        }
    }
    if (frame != 0) {
        // send half packet
        packets_sent++;
        ret = gfs_submit_slab(packet, K_FOREVER);
        if (ret != 0) {
            LOG_WRN("Couldnt submit final slab: %d", ret);
        }
    }
    int64_t total_elapsed = k_cycle_get_64() - total_start;

    int64_t end = k_uptime_get();
    int64_t elapsed = end - start;
    float perSec = packets_sent / (elapsed / 1000.0f);

    LOG_INF("Its all over: %d packets in %lld ms. %f per second", packets_sent, elapsed, (double) perSec);
    printk("IMU, Barom, Slab, sync, boost, all \n");
    printk("%lld, %lld, %lld, %lld, %lld, %lld\n", cyc_read_imu, cyc_read_barom, cyc_slabs, cyc_syncing, cyc_calc_boost,
           total_elapsed);
    return 0;
}

int cmd_unlock(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "Unlocking boost data");
    unlock_boostdata();
    shell_print(shell, "Success, rebooting....\n\n\n");
    sys_reboot(SYS_REBOOT_COLD);
}

int cmd_stop(const struct shell *shell, size_t argc, char **argv) {
    DONT_STOP = false;
    shell_print(shell, "Stopping");
    return 0;
}

int cmd_readall(const struct shell *shell, size_t argc, char **argv) {
    SuperFastPacket pac;
    shell_fprintf_normal(shell, "ts, temp, press, ax, ay, az, gx, gy, gz\n");

    for (int i = 0; i < gfs_total_blocks(); i++) {
        int ret = gfs_read_block(i, &pac);
        if (ret != 0) {
            LOG_WRN("Couldnt read page # %d: %d", i, ret);
        }
        for (int j = 0; j < 10; j++) {
            if (j == 0) {
                shell_fprintf_normal(shell, "%lld, %f, %f,", pac.timestamp, (double) pac.temp, (double) pac.pressure);
            } else {
                shell_fprintf_normal(shell, "NaN, NaN, NaN,");
            }
            shell_fprintf_normal(shell, "%f, %f, %f, %f, %f, %f\n", (double) pac.adat[j].ax, (double) pac.adat[j].ay,
                                 (double) pac.adat[j].az, (double) pac.gdat[j].gx, (double) pac.gdat[j].gy,
                                 (double) pac.gdat[j].gz);
        }
        k_msleep(10);
    }
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(test_subcmds, SHELL_CMD(stop, NULL, "Stop Test", cmd_stop),
                               SHELL_CMD(dumpgorb, NULL, "Dump Gorb partition", cmd_readall),
                               SHELL_CMD(unlocl, NULL, "Unlock flight data partition", cmd_unlock),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(test, &test_subcmds, "Test Commands", NULL);
