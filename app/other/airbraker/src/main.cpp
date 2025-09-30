#include "5v_ctrl.h"
#include "boost.h"
#include "common.h"
#include "f_core/os/c_datalogger.h"
#include "f_core/utils/linear_fit.hpp"
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

const struct device *ina_servo = DEVICE_DT_GET(DT_ALIAS(inaservo));

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
    rail_item_enable(FiveVoltItem::Servos);

    const struct device *imu_dev = DEVICE_DT_GET(DT_ALIAS(imu));
    const struct device *barom_dev = DEVICE_DT_GET(DT_ALIAS(barom));

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

const float max_open = 2500;
const float max_closed = 500;

uint32_t last_pulselen = PWM_USEC(500);

int disconnect() { return pwm_set_pulse_dt(&servo, 0); }

int set_servo(uint32_t pulse) {
    last_pulselen = pulse;
    return pwm_set_pulse_dt(&servo, pulse);
}

float current_cutoff = 0.75; 
const float max_current_cutoff = 2.4;

float current(const struct shell *shell) {
    int ret = sensor_sample_fetch(ina_servo);
    if (ret != 0) {
        shell_warn(shell, "FAILED TO READ INA: %d", ret);
        return 0;
    }
    struct sensor_value val;
    ret = sensor_channel_get(ina_servo, SENSOR_CHAN_CURRENT, &val);
    if (ret != 0) {
        shell_warn(shell, "FAILED TO READ INA: %d", ret);
        return 0;
    }
    float f = sensor_value_to_float(&val);
    if (f > 75){
        f = 0.5;
    }
    return f;
}

void beep() {
    for (int i = 0; i < 5; i++) {
        rail_item_enable(FiveVoltItem::Buzzer);
        k_msleep(100);
        rail_item_disable(FiveVoltItem::Buzzer);
        k_msleep(100);
    }
}

void sweep_servo(const struct shell *shell, uint32_t next_pulse) {
    const uint32_t start_pulse = last_pulselen;

    int delta = (int) next_pulse - (int) start_pulse;
    const float max_step = 20*1000;// 20 us full rev in 
    int steps = delta / max_step;
    int sgn = delta > 0 ? 1 : -1;
    CMovingAverage<float, 20> avg{0};

    for (int i = 0; i < abs(steps); i++) {
        uint32_t pulse = start_pulse + (sgn * max_step) * i;
        float I = current(shell);
        avg.Feed(I);
        if (avg.Avg() > current_cutoff) {
            shell_error(shell, "AGHGGHGHGH OVERCURRENT: sample; %f, avg: %f", (double)I, (double)avg.Avg());
            disconnect();
            beep();
            break;
        }
        shell_print(shell, "Pulse: %d: %f", pulse, (double) I);
        int ret = set_servo(pulse);
        if (ret != 0) {
            shell_error(shell, "Failed to set servo");
        }
        k_msleep(10);
    }
    k_msleep(20);
    disconnect();
}

int cmd_move(const struct shell *shell, size_t argc, char **argv) {
    if (argc < 2) {
        shell_error(shell, "Specify a degree to target to (0 - 270)");
        return -1;
    }
    int angle = atoi(argv[1]);
    if (angle > 270 || angle < 0){
        shell_error(shell, "Invalid angle: 0-270");
        return -1;
    }
    float as_pct = angle / 270.f;
    float us = max_closed * (1-as_pct) + max_open*as_pct;

    rail_item_enable(FiveVoltItem::Servos);
    sweep_servo(shell, PWM_USEC((int)us));
    rail_item_disable(FiveVoltItem::Servos);

    return 0;
}
int cmd_get_limit(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "Current Limit: %d mA over 200ms", (int)(current_cutoff*1000));
    return 0;
}

int cmd_set_limit(const struct shell *shell, size_t argc, char **argv) {
        if (argc < 2) {
        shell_error(shell, "Specify a current cutoff in mA (no greater than 2400 mA)");
        return -1;
    }
    int ma = atoi(argv[1]);
    float amps = (float)ma/1000.f;
    if (amps > max_current_cutoff){
        shell_error(shell, "Bad mece, dont set it that high");
        return -1;
    }
    current_cutoff = amps;
    return 0;
}


SHELL_STATIC_SUBCMD_SET_CREATE(servo_subcmds, 
SHELL_CMD(move, NULL, "Move servo to an angle 0 to 270", cmd_move), 
SHELL_CMD(set_limit, NULL, "Set servo current limit (milliamps over 200ms)", cmd_set_limit),
SHELL_CMD(get_limit, NULL, "Get servo current limit (milliamps over 200ms)", cmd_get_limit),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(servo, &servo_subcmds, "Servo Commands", NULL);
