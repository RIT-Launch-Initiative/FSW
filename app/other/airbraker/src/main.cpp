#include "5v_ctrl.h"
#include "boost.h"
#include "common.h"
#include "f_core/os/c_datalogger.h"
#include "fast_sensing.h"
#include "flight.h"
#include "gorbfs.h"
#include "model.hpp"
#include "storage.h"

#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
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


#define SERVO_EN DT_NODELABEL(servo_enable)
static const struct gpio_dt_spec servo_en = GPIO_DT_SPEC_GET(SERVO_EN, gpios);

int main() {
    int ret = 0;
    if (!gpio_is_ready_dt(&servo_en)) {
        printk("No servo enable :(\n");
        return 0;
    }
    ret = gpio_pin_configure_dt(&servo_en, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Failed to conf servo_en :(\n");
        return 0;
    }

    ret = gpio_pin_set_dt(&servo_en, 1);
    if (ret < 0) {
        printk("couldnt set servo_en: %d\n", ret);
    }

    const struct device *imu_dev = DEVICE_DT_GET(DT_ALIAS(imu));
    const struct device *barom_dev = DEVICE_DT_GET(DT_ALIAS(barom));
    const struct device *ina_servo = DEVICE_DT_GET(DT_ALIAS(inaservo));

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
    return 0;
    //Ground, Boost, Coast, Flight
    ret = boost_and_flight_sensing(imu_dev, barom_dev, ina_servo);
    LOG_INF("On the ground now");

    return 0;
}
struct pwm_dt_spec servo = PWM_DT_SPEC_GET(DT_ALIAS(servo1));
const uint32_t open_pulselen PWM_USEC(930);
const uint32_t closed_pulselen = PWM_USEC(2010);
uint32_t last_pulselen = 0;

int disconnect() { return pwm_set_pulse_dt(&servo, 0); }

int set_servo(uint32_t pulse) {
    last_pulselen = pulse;
    return pwm_set_pulse_dt(&servo, pulse);
}
void sweep_servo(uint32_t next_pulse) {
    const int steps = 100;
    const uint32_t start_pulse = last_pulselen;

    int delta = (int) next_pulse - (int) start_pulse;
    int step = delta / steps;

    for (int i = 0; i < steps; i++) {
        uint32_t pulse = start_pulse + step * i;
        printk("Pulse: %d\n", pulse);
        int ret = set_servo(pulse);
        if (ret != 0) {
            LOG_WRN("Failed to set servo");
        }
        k_msleep(10);
    }
    disconnect();
}

int cmd_move(const struct shell *shell, size_t argc, char **argv) {
    if (argc < 2) {
        shell_error(shell, "Specify microsecond value for PWM. 0-3000 give or take");
        return -1;
    }
    int pulse_us = atoi(argv[1]);

    rail_item_enable(FiveVoltItem::Servos);
    sweep_servo(PWM_USEC(pulse_us));
    // rail_item_disable(FiveVoltItem::Servos);

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(servo_subcmds, SHELL_CMD(move, NULL, "Move servo to the us value", cmd_move),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(servo, &servo_subcmds, "Servo Commands", NULL);
