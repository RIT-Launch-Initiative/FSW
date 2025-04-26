#include "f_core/radio/protocols/horus/horus.h"
#include "orient.h"
// #include "ublox_m10.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/types.h>

#define IMU_NODE DT_ALIAS(imu)
static const struct device *imu_dev = DEVICE_DT_GET(IMU_NODE);

#define PUMPEN_NODE DT_NODELABEL(pump_enable)
static const struct gpio_dt_spec pump_enable = GPIO_DT_SPEC_GET(PUMPEN_NODE, gpios);

extern int cmd_servo_sweep(const struct shell *shell, size_t argc, char **argv);
extern int cmd_servo_move(const struct shell *shell, size_t argc, char **argv);
extern int cmd_servo_off(const struct shell *shell, size_t argc, char **argv);
extern int cmd_servo_on(const struct shell *shell, size_t argc, char **argv);
extern int cmd_servo_freak(const struct shell *shell, size_t argc, char **argv);
extern int cmd_servo_try_righting(const struct shell *shell, size_t argc, char **argv);

int cmd_pump_off(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(shell);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    printk("Pump Off\n");
    return gpio_pin_set_dt(&pump_enable, 0);
}
int cmd_pump_on(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(shell);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    printk("Pump On\n");
    return gpio_pin_set_dt(&pump_enable, 1);
}

int cmd_orient(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(shell);
    ARG_UNUSED(argv);
    ARG_UNUSED(argc);
    vec3 me = {0, 0, 0};
    int ret = find_vector(me);
    (void) ret;
    PayloadFace to_actuate = find_orientation(me);
    if (to_actuate == PayloadFace::Upright) {
        shell_print(shell, "Already upright");
    } else if (to_actuate == PayloadFace::OnItsHead || to_actuate == PayloadFace::StandingUp) {
        shell_print(shell, "We're boned");
    } else {
        shell_print(shell, "To actuate: %d", to_actuate);
        // look up servo
    }
    return 0;
}

extern void init_lora_modem();
extern int init_servo();

#define SERVO_EN DT_NODELABEL(servo_enable)
static const struct gpio_dt_spec servo_en = GPIO_DT_SPEC_GET(SERVO_EN, gpios);

#define LDO5V_EN DT_NODELABEL(ldo5v_enable)
static const struct gpio_dt_spec ldo5v_en = GPIO_DT_SPEC_GET(LDO5V_EN, gpios);

#define BUZZ_EN DT_NODELABEL(buzzer)
static const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(BUZZ_EN, gpios);

// clang-format off

SHELL_STATIC_SUBCMD_SET_CREATE(freak_subcmds, 
        SHELL_CMD(orient, NULL, "Orientation info", cmd_orient),
        SHELL_CMD(roll, NULL, "Attempt to flip: [# of attempts]", cmd_servo_try_righting),
        SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(freak, &freak_subcmds, "Control Freak Control Commands", NULL);

SHELL_STATIC_SUBCMD_SET_CREATE(servo_subcmds, 
        SHELL_CMD(sweep, NULL, "sweep servo", cmd_servo_sweep),
        SHELL_CMD(move, NULL, "move servo", cmd_servo_move),
        SHELL_CMD(on, NULL, "Turn on servo power", cmd_servo_on),
        SHELL_CMD(off, NULL, "Turn off servo power", cmd_servo_off),
        SHELL_CMD(freak, NULL, "Freak servos", cmd_servo_freak),
        SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(servo, &servo_subcmds, "Servo Commands", NULL);

SHELL_STATIC_SUBCMD_SET_CREATE(pump_subcmds, 
        SHELL_CMD(on, NULL, "Pump on", cmd_pump_on),
        SHELL_CMD(off, NULL, "Pump off", cmd_pump_off),
        SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(pump, &pump_subcmds, "Pump Commands", NULL);

extern const struct device *ina;
void buzzer_on(){
    gpio_pin_set_dt(&buzzer, 1);
}
void buzzer_off(){
    gpio_pin_set_dt(&buzzer, 0);
}

bool BEEP_FOREVER_IF_LOWBAT(){
    struct sensor_value voltage = {0};
    sensor_sample_fetch(ina);
    sensor_channel_get(ina, SENSOR_CHAN_VOLTAGE, &voltage);
    float c = sensor_value_to_float(&voltage);
    // all good
    if (c > 7.0){
        return true;
    }
    while(true){
        for (int i = 0; i < 6; i++){
            buzzer_on();
            k_msleep(100);
            buzzer_off();
            k_msleep(100);
        }
        buzzer_on();
        k_msleep(15000);
        buzzer_off();
    }
    return false;
}



int right_until_upright(){
    bool upright = do_righting_once();
    if (upright){
        printf("chilling out\n");
        // just chill out man
        return 0;
    }
    while (true){
        BEEP_FOREVER_IF_LOWBAT();

        upright = do_righting_once();
        printf("while truing\n");

        if (upright){
            gpio_pin_set_dt(&pump_enable, 1);
            k_msleep(5000);
            gpio_pin_set_dt(&pump_enable, 0);
            break;
        }
    }
    return 0;
}



int main() {
    struct sensor_value sampling = {0};
     sensor_value_from_float(&sampling, 208);
    int ret = sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling);
    if (ret < 0){
        printk("Couldnt set sampling\n");
    }
    init_servo();

    if (!gpio_is_ready_dt(&pump_enable)) {
        printk("No pump pin :(\n");
        return 0;
    }

    ret = gpio_pin_configure_dt(&pump_enable, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("Failed to conf pump pin:(\n");
        return 0;
    }

    int upside_down_count = 0;
    while(true){
        vec3 my_dir = {0};
        find_vector(my_dir);
        PayloadFace facing = find_orientation(my_dir);
        
        if (facing == PayloadFace::OnItsHead){
            upside_down_count++;
        } else {
            upside_down_count = 0;
        }

        if (upside_down_count > 5){
            for (int i = 0; i < 5; i++){
                buzzer_on();
                k_msleep(100);
                buzzer_off();
                k_msleep(100);
            }
            k_msleep(3000);
            right_until_upright();
            upside_down_count = 0;
        }
        k_msleep(100);
    }


    return 0;
}
