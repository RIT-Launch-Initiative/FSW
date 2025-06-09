#include "f_core/radio/protocols/horus/horus.h"
// #include "ublox_m10.h"

#include "5v_ctrl.h"
#include "flipping.h"

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

#define INA_NODE DT_ALIAS(inaservo)
static const struct device *ina_dev = DEVICE_DT_GET(INA_NODE);

#define PUMPEN_NODE DT_NODELABEL(pump_enable)
static const struct gpio_dt_spec pump_enable = GPIO_DT_SPEC_GET(PUMPEN_NODE, gpios);

int cmd_pump_off(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(shell);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    printk("Pump Off\n");
    rail_item_disable(FiveVoltItem::Pump);

    return 0;
}
int cmd_pump_on(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(shell);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    printk("Pump On\n");
    rail_item_enable(FiveVoltItem::Pump);
    return 0;
}

bool doflip = false;

int cmd_orient(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(shell);
    ARG_UNUSED(argv);
    ARG_UNUSED(argc);
    doflip = true;
    return 0;
}

#define SERVO_EN DT_NODELABEL(servo_enable)
static const struct gpio_dt_spec servo_en = GPIO_DT_SPEC_GET(SERVO_EN, gpios);

#define LDO5V_EN DT_NODELABEL(ldo5v_enable)
static const struct gpio_dt_spec ldo5v_en = GPIO_DT_SPEC_GET(LDO5V_EN, gpios);

#define BUZZ_EN DT_NODELABEL(buzzer)
static const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(BUZZ_EN, gpios);

// clang-format off

SHELL_STATIC_SUBCMD_SET_CREATE(freak_subcmds, 
        SHELL_CMD(orient, NULL, "Orientation info", cmd_orient),
        SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(freak, &freak_subcmds, "Control Freak Control Commands", NULL);


SHELL_STATIC_SUBCMD_SET_CREATE(pump_subcmds, 
        SHELL_CMD(on, NULL, "Pump on", cmd_pump_on),
        SHELL_CMD(off, NULL, "Pump off", cmd_pump_off),
        SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(pump, &pump_subcmds, "Pump Commands", NULL);


void buzzer_on(){
    rail_item_enable(FiveVoltItem::Buzzer);
}
void buzzer_off(){
    rail_item_disable(FiveVoltItem::Buzzer);
}

bool BEEP_FOREVER_IF_LOWBAT(){
    struct sensor_value voltage = {0};
    sensor_sample_fetch(ina_dev);
    sensor_channel_get(ina_dev, SENSOR_CHAN_VOLTAGE, &voltage);
    float c = sensor_value_to_float(&voltage);
    // all good
    if (c > 7.f){
        return true;
    }
    while(true){
        for (int i = 0; i < 6; i++){
            rail_item_enable(FiveVoltItem::Buzzer);
            k_msleep(100);
            rail_item_disable(FiveVoltItem::Buzzer);
            k_msleep(100);
        }
        rail_item_enable(FiveVoltItem::Buzzer);
        k_msleep(15000);
        rail_item_disable(FiveVoltItem::Buzzer);
    }
    return false;
}


int read_imu(const struct device *imu_dev, NTypes::AccelerometerData &acc) {
    int ret = sensor_sample_fetch(imu_dev);
    if (ret != 0) {
        return ret;
    }
    struct sensor_value vals[3];
    ret = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, vals);
    if (ret != 0) {
        return ret;
    }
    acc.X = sensor_value_to_float(&vals[0]);
    acc.Y = sensor_value_to_float(&vals[1]);
    acc.Z = sensor_value_to_float(&vals[2]);


    return 0;
}


void find_vector_norm(NTypes::AccelerometerData &vec){
    read_imu(imu_dev, vec);
    float mag = vec.X * vec.X + vec.Y * vec.Y + vec.Z * vec.Z;
    vec.X /= mag;
    vec.Y /= mag;
    vec.Z /= mag;
}

int flip_one_side(const struct device *ina_dev, const Servo &servo, SweepStrategy strat, bool open, bool hold = false);


extern const Servo *servos[];
bool do_righting_once(){
    rail_item_enable(FiveVoltItem::Servos);
    NTypes::AccelerometerData vec = {0};
    find_vector_norm(vec); 
    PayloadFace face = find_orientation(vec);
    if (face > PayloadFace::Face3){
        printk("Cant do anything about that");
        return false;
    }
    // get associated servo
    // actuate out
    flip_one_side(ina_dev, *servos[face], SweepStrategy::Fast, true);
    // actuate in
    flip_one_side(ina_dev, *servos[face], SweepStrategy::Fast, false);
    // measure IMU
    // return true if on front face
    find_vector_norm(vec); 
    face = find_orientation(vec);

    return face == PayloadFace::Upright;
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
            k_msleep(5000);
            break;
        }
    }
    return 0;
}

extern int init_flip_hw();
int main() {
    struct sensor_value sampling = {0};
     sensor_value_from_float(&sampling, 208);
    int ret = sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling);
    if (ret < 0){
        printk("Couldnt set sampling\n");
    }
     init_flip_hw();

    
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
        NTypes::AccelerometerData my_dir = {0};
        find_vector_norm(my_dir);
        PayloadFace facing = find_orientation(my_dir);
        

        if (doflip){
            for (int i = 0; i < 5; i++){
                buzzer_on();
                k_msleep(100);
                buzzer_off();
                k_msleep(100);
            }
            k_msleep(3000);
            right_until_upright();
            doflip = false;
        }
        k_msleep(100);
    }


    return 0;
}
