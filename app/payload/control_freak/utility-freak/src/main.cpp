#include "5v_ctrl.h"
#include "cameras.h"
#include "common.h"
#include "flipping.h"

#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/base64.h>
#include <zephyr/sys/reboot.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_FREAK_LOG_LEVEL);

#include <zephyr/kernel.h>

int main() {
    int ret = 0;

    ret = five_volt_rail_init();
    if (ret != 0) {
        LOG_ERR("Failed to init 5v rail control");
    }

    const struct device *imu_dev = DEVICE_DT_GET(DT_ALIAS(imu));
    const struct device *barom_dev = DEVICE_DT_GET(DT_ALIAS(barom));

    const struct device *ina_servo = DEVICE_DT_GET(DT_ALIAS(inaservo));
    const struct device *ina_pump = DEVICE_DT_GET(DT_ALIAS(inapump));

    if (!device_is_ready(imu_dev) || !device_is_ready(barom_dev)) {
        LOG_ERR("Sensor devices not ready");
        return -1;
    }
    if (!device_is_ready(ina_servo) || !device_is_ready(ina_pump)) {
        LOG_ERR("Sensor devices not ready");
        return -1;
    }

    ret = init_flip_hw();
    if (ret != 0) {
        LOG_ERR("Error initializing servo hardware");
    }
    rail_item_enable(FiveVoltItem::Servos);
    return 0;
}

// SHELL_STATIC_SUBCMD_SET_CREATE(test_subcmds, SHELL_CMD(stop, NULL, "Stop Test", cmd_stop),
//    SHELL_CMD(unlocl, NULL, "Unlock flight data partition", cmd_unlock),
//    SHELL_CMD(boost, NULL, "fake boost detect", cmd_boost),
//    SHELL_CMD(inflated, NULL, "fake inflated", cmd_inflated), SHELL_SUBCMD_SET_END);
//
// SHELL_CMD_REGISTER(test, &test_subcmds, "Test Commands", NULL);
//
// SHELL_STATIC_SUBCMD_SET_CREATE(gorb_subcmds, SHELL_CMD(dumpfast, NULL, "Dump fast partition", cmd_readfast),
//    SHELL_CMD(dumpfast, NULL, "Dump slow partition", cmd_readslow), SHELL_SUBCMD_SET_END);
//
// SHELL_CMD_REGISTER(gorb, &gorb_subcmds, "Gorbfs Commands", NULL);