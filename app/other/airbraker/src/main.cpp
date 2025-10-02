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

#include <math.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/base64.h>
#include <zephyr/sys/reboot.h>
#include "effort.h"
LOG_MODULE_REGISTER(main, CONFIG_APP_AIRBRAKE_LOG_LEVEL);

#include <zephyr/kernel.h>
constexpr float BATTERY_STOP_THRESH = 7.0;
float startup_voltage = 0;

const struct device *ina_servo = DEVICE_DT_GET(DT_ALIAS(inaservo));

#define SERVO_EN DT_NODELABEL(servo_enable)
static const struct gpio_dt_spec servo_en = GPIO_DT_SPEC_GET(SERVO_EN, gpios);

struct pwm_dt_spec servo = PWM_DT_SPEC_GET(DT_ALIAS(servo1));

const float max_open = 2500;
const float max_closed = 500;
bool STOP_RIGHT_NOW = false;

float current_cutoff = 0.75;
const float max_current_cutoff = 2.4;

void restart() {
    int ret = gpio_pin_set_dt(&servo_en, 1);
    if (ret < 0) {
        printk("couldnt set servo_en: %d\n", ret);
    }
    rail_item_enable(FiveVoltItem::Servos);
}

int disconnect() { return pwm_set_pulse_dt(&servo, 0); }
int depower() {
    int ret = gpio_pin_set_dt(&servo_en, 9);
    if (ret < 0) {
        printk("couldnt set servo_en: %d\n", ret);
    }
    rail_item_disable(FiveVoltItem::Servos);
    return ret;
}

void beep() {
    for (int i = 0; i < 5; i++) {
        rail_item_enable(FiveVoltItem::Buzzer);
        k_msleep(100);
        rail_item_disable(FiveVoltItem::Buzzer);
        k_msleep(100);
    }
}

uint32_t last_pulselen = PWM_USEC(500);

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

    if (!device_is_ready(ina_servo)) {
        LOG_ERR("Sensor devices not ready");
        return -1;
    }
    float current = 0;

    ret = read_ina(ina_servo, startup_voltage, current);
    if (ret != 0) {
        LOG_ERR("Couldn't read battery");
    }
    if (startup_voltage < BATTERY_STOP_THRESH) {
        beep();
        printk("LOW VOLTAGE");
        return -1;
    }
    CMovingAverage<float, 20> avg{0};
    CMovingAverage<float, 10> vavg{7.4};
    const int print_every = 1;
    size_t i = 0;
    while (true) {
        float V = 0;
        float I = 0;

        int ret = read_ina(ina_servo, V, I);
        if (ret != 0) {
            LOG_WRN("Couldnt read ina: %d", ret);
        }
        avg.Feed(I);
        vavg.Feed(V);

        if (avg.Avg() > current_cutoff) {
            STOP_RIGHT_NOW = true;
            disconnect();
            depower();
            printk("STOPPING BC OVERCURRENT\n");
            beep();
        }
        if (vavg.Avg() < BATTERY_STOP_THRESH) {
            STOP_RIGHT_NOW = true;
            disconnect();
            depower();
            printk("LOW VOLTAGE");
            beep();
            return -1;
        }

        if (i % print_every == 0) {
            printk("M: %lld, P: %d, I: %.4f, Ia: %.4f, V: %.4f\n", k_uptime_get(), last_pulselen, (double) I, (double) avg.Avg(), (double) V);
        }

        k_msleep(10);
        i++;
    }

    return 0;
}


int set_servo(uint32_t pulse) {
    last_pulselen = pulse;
    return pwm_set_pulse_dt(&servo, pulse);
}

void sweep_servo(const struct shell *shell, uint32_t next_pulse) {
    const uint32_t start_pulse = last_pulselen;

    int delta = (int) next_pulse - (int) start_pulse;
    const float max_step = 20 * 1000; // 20 us full rev in
    int steps = delta / max_step;
    int sgn = delta > 0 ? 1 : -1;

    // MOVING_NOW = true;
    for (int i = 0; i < abs(steps); i++) {
        if (STOP_RIGHT_NOW) {
            rail_item_disable(FiveVoltItem::Servos);
            depower();
            disconnect();
            // MOVING_NOW = false;
            return;
        }
        uint32_t pulse = start_pulse + (sgn * max_step) * i;
        int ret = set_servo(pulse);
        if (ret != 0) {
            shell_error(shell, "Failed to set servo");
        }
        k_msleep(10);
    }
    k_msleep(20);
    disconnect();
    // MOVING_NOW = false;
}

int cmd_move(const struct shell *shell, size_t argc, char **argv) {
    if (STOP_RIGHT_NOW) {
        shell_error(shell, "NOT STARTING BC TOLD TO STOP. USE RESET TO RESTART");
        return -1;
    }
    if (argc < 2) {
        shell_error(shell, "Specify a degree to target to (0 - 270)");
        return -1;
    }
    int angle = atoi(argv[1]);
    if (angle < 0) {
        shell_print(shell, "DCing");
        disconnect();
        depower();
        return 0;
    }
    if (angle > 270 || angle < 0) {
        shell_error(shell, "Invalid angle: 0-270");
        return -1;
    }
    float as_pct = angle / 270.f;
    float us = max_closed * (1 - as_pct) + max_open * as_pct;
    gpio_pin_set_dt(&servo_en, 1);
    rail_item_enable(FiveVoltItem::Servos);
    set_servo(PWM_USEC(us));

    return 0;
}

uint32_t effort_to_pulse(float effort) {
    float most_open_angle = 70.f;
    float angle = effort * most_open_angle;
    float as_pct = angle / 270.f;
    float us = max_closed * (1.f - as_pct) + max_open * as_pct;
    return PWM_USEC(us);
}

int cmd_effort_sim(const struct shell *shell, size_t argc, char **argv) {
    for (int i = 0; i < sizeof(effort_lut)/sizeof(effort_lut[0]); i++) {
        if (STOP_RIGHT_NOW) {
            rail_item_disable(FiveVoltItem::Servos);
            depower();
            disconnect();
            // MOVING_NOW = false;
            return -1;
        }
        float effort = effort_lut[i];

        set_servo(effort_to_pulse(effort));
        k_msleep(10);
    }
    return 0;
}



int cmd_wobble(const struct shell *shell, size_t argc, char **argv) {
    for (int i = 0; i < 10000; i++) {
        if (STOP_RIGHT_NOW) {
            rail_item_disable(FiveVoltItem::Servos);
            depower();
            disconnect();
            // MOVING_NOW = false;
            return -1;
        }
        float t = 4 * (float) i / 1000.f;
        float y = sin(t);
        float effort = y / 2.f + .5f;

        set_servo(effort_to_pulse(effort));
        k_msleep(1);
    }
    return 0;
}

int cmd_get_limit(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "Current Limit: %d mA over 200ms", (int) (current_cutoff * 1000));
    return 0;
}

int cmd_set_limit(const struct shell *shell, size_t argc, char **argv) {
    if (argc < 2) {
        shell_error(shell, "Specify a current cutoff in mA (no greater than 2400 mA)");
        return -1;
    }
    int ma = atoi(argv[1]);
    float amps = (float) ma / 1000.f;
    if (amps > max_current_cutoff) {
        shell_error(shell, "Bad mece, dont set it that high");
        return -1;
    }
    current_cutoff = amps;
    return 0;
}

int cmd_reset(const struct shell *shell, size_t argc, char **argv) {
    shell_error(shell, "MAKE SURE YOU ARENT STALLING ANYMORE");
    STOP_RIGHT_NOW = false;
    gpio_pin_set_dt(&servo_en, 1);
    rail_item_enable(FiveVoltItem::Servos);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    servo_subcmds, SHELL_CMD(move, NULL, "Move servo to an angle 0 to 270", cmd_move),
    SHELL_CMD(wobble, NULL, "Wobble for 10s", cmd_wobble),
    SHELL_CMD(sim, NULL, "Sim for ~12s", cmd_effort_sim),
    SHELL_CMD(set_limit, NULL, "Set servo current limit (milliamps over 200ms)", cmd_set_limit),
    SHELL_CMD(get_limit, NULL, "Get servo current limit (milliamps over 200ms)", cmd_get_limit),
    SHELL_CMD(reset, NULL, "Reset servo after overcurrent", cmd_reset), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(servo, &servo_subcmds, "Servo Commands", NULL);
