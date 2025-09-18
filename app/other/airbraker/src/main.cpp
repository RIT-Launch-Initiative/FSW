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

int main() {
    int ret = 0;
    ret = five_volt_rail_init();
    if (ret != 0) {
        LOG_ERR("Failed to init 5v rail control");
        // buzzer_tell(BuzzCommand::SensorTroubles);
    }

    const struct device *imu_dev = DEVICE_DT_GET(DT_ALIAS(imu));
    const struct device *barom_dev = DEVICE_DT_GET(DT_ALIAS(barom));

    const struct device *ina_servo =    DEVICE_DT_GET(DT_ALIAS(ina_servo));

    if (!device_is_ready(imu_dev) || !device_is_ready(barom_dev)) {
        LOG_ERR("Sensor devices not ready");
        // buzzer_tell(BuzzCommand::SensorTroubles);
        return -1;
    }
    if (!device_is_ready(ina_servo)) {
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

    //Ground, Boost, Coast, Flight
    ret = boost_and_flight_sensing(imu_dev, barom_dev, ina_servo);
    LOG_INF("On the ground now");

    return 0;
}
