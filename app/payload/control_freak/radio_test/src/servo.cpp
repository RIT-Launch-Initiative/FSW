#include "orient.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

static constexpr uint32_t min_pulse = PWM_USEC(800);  //DT_PROP(DT_PARENT(DT_A LIAS(servo1)), min_pulse);
static constexpr uint32_t max_pulse = PWM_USEC(1700); //DT_PROP(DT_PARENT(DT_ALIAS(servo1)), max_pulse);

#define INA_NODE DT_ALIAS(inaservo)
const struct device *ina = DEVICE_DT_GET(INA_NODE);

// false when closed, true when open

bool state1 = 0;
bool state2 = 0;
bool state3 = 0;

static constexpr Servo Servo1{
    .pwm = PWM_DT_SPEC_GET(DT_ALIAS(servo1)),
    .open_pulselen = min_pulse,
    .closed_pulselen = max_pulse,
    .state = state1,
};
static constexpr Servo Servo2{
    .pwm = PWM_DT_SPEC_GET(DT_ALIAS(servo2)),
    .open_pulselen = max_pulse,
    .closed_pulselen = min_pulse,
    .state = state2,
};
static constexpr Servo Servo3{
    .pwm = PWM_DT_SPEC_GET(DT_ALIAS(servo3)),
    .open_pulselen = max_pulse,
    .closed_pulselen = min_pulse,
    .state = state3,
};

const Servo *servos[] = {&Servo1, &Servo2, &Servo3};

void change_servo(const Servo &servo, bool newstate) {
    servo.state = newstate;
    uint32_t pulse = servo.closed_pulselen;
    if (servo.state) {
        pulse = servo.open_pulselen;
    };

    int ret = pwm_set_pulse_dt(&servo.pwm, pulse);
    if (ret < 0) {
        printk("Error %d: failed to set pulse width\n", ret);
        return;
    }
}

struct Power {
    float current;
    float voltage;
    float power;
};
Power current_log[120] = {};
void sweep_servo(const Servo &servo, bool newstate) {
    servo.state = newstate;
    uint32_t start = servo.closed_pulselen;
    uint32_t end = servo.open_pulselen;
    if (servo.state) {
        start = servo.open_pulselen;
        end = servo.closed_pulselen;
    };

    int steps = 120;
    int delta = end - start;
    printf("%d to %d\n", start, end);
    for (int i = 0; i < steps; i++) {
        struct sensor_value current;
        sensor_sample_fetch(ina);
        sensor_channel_get(ina, SENSOR_CHAN_CURRENT, &current);
        float c = sensor_value_to_float(&current);
        current_log[i].current = c;

        sensor_sample_fetch(ina);
        sensor_channel_get(ina, SENSOR_CHAN_VOLTAGE, &current);
        c = sensor_value_to_float(&current);
        current_log[i].voltage = c;

        sensor_sample_fetch(ina);
        sensor_channel_get(ina, SENSOR_CHAN_POWER, &current);
        c = sensor_value_to_float(&current);
        current_log[i].power = c;

        int pulse = start + delta * i / steps;

        int ret = pwm_set_pulse_dt(&servo.pwm, pulse);
        if (ret < 0) {
            printk("Error %d: failed to set pulse width\n", ret);
        }
        k_msleep(20);
    }
    printf("Current, Voltage, Power\n");
    for (int i = 0; i < steps; i++) {
        printf("%.4f, %.4f, %.4f\n", current_log[i].current, current_log[i].voltage, current_log[i].power);
    }
    int ret = pwm_set_pulse_dt(&servo.pwm, end);
    if (ret < 0) {
        printk("Error %d: failed to set pulse width\n", ret);
    }
    k_msleep(100);
    ret = pwm_set_pulse_dt(&servo.pwm, 0);
    if (ret < 0) {
        printk("Error %d: failed to set pulse width\n", ret);
    }
}

#define SERVO_EN DT_NODELABEL(servo_enable)
static const struct gpio_dt_spec servo_en = GPIO_DT_SPEC_GET(SERVO_EN, gpios);

#define LDO5V_EN DT_NODELABEL(ldo5v_enable)
static const struct gpio_dt_spec ldo5v_en = GPIO_DT_SPEC_GET(LDO5V_EN, gpios);

int init_servo() {
    int ret;
    if (!gpio_is_ready_dt(&servo_en)) {
        printk("No servo enable :(\n");
        return 0;
    }
    if (!gpio_is_ready_dt(&ldo5v_en)) {
        printk("No ldo5v enable :(\n");
        return 0;
    }
    ret = gpio_pin_configure_dt(&servo_en, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Failed to conf servo_en :(\n");
        return 0;
    }
    ret = gpio_pin_configure_dt(&ldo5v_en, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Failed to conf ldo5v_en :(\n");
        return 0;
    }
    ret = gpio_pin_set_dt(&servo_en, 1);
    if (ret < 0) {
        printk("couldnt set servo_en: %d\n", ret);
    }

    ret = gpio_pin_set_dt(&ldo5v_en, 1);
    if (ret < 0) {
        printk("couldnt set ldo5v_en: %d\n", ret);
    }
    printk("Servos on\n");
    return 0;
}
int cmd_servo_freak(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(shell);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    for (int i = 0; i < 10; i++) {
        change_servo(Servo1, false);
        k_msleep(1000);
        change_servo(Servo2, false);
        k_msleep(1000);
        change_servo(Servo3, false);
        k_msleep(1000);

        change_servo(Servo1, true);
        k_msleep(1000);
        change_servo(Servo2, true);
        k_msleep(1000);
        change_servo(Servo3, true);
        k_msleep(1000);
    }
    return 0;
}
int cmd_servo_on(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(shell);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    return init_servo();
}
int cmd_servo_off(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(shell);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    int ret = gpio_pin_set_dt(&servo_en, 0);
    if (ret < 0) {
        printk("couldnt set servo_en: %d\n", ret);
    }

    ret = gpio_pin_set_dt(&ldo5v_en, 0);
    if (ret < 0) {
        printk("couldnt set ldo5v_en: %d\n", ret);
    }
    return ret;
}

int parse_servo_args(const struct shell *shell, size_t argc, char **argv, int *const servoNum, bool *const state) {
    if (argc != 2 && argc != 3) {
        shell_print(shell, "Wrong number of args");
        shell_print(shell, "Help: servo 1/2/3 o/c");
        return -1;
    }
    char servoNumChar = argv[1][0];

    *servoNum = 0;
    if (servoNumChar == '1') {
        *servoNum = 0;
    } else if (servoNumChar == '2') {
        *servoNum = 1;
    } else if (servoNumChar == '3') {
        *servoNum = 2;
    } else {
        shell_print(shell, "Invalid servo number");
        return -1;
    }
    if (argc == 2) {
        *state = !(servos[*servoNum]->state);
        change_servo(*servos[*servoNum], *state);

    } else if (argc == 3) {

        char operationChar = argv[2][0];
        shell_print(shell, "Servo Control %c %c", servoNumChar, operationChar);
        *state = true;

        if (operationChar == 'o') {
            *state = true;
        } else if (operationChar == 'c') {
            *state = false;
        } else {
            shell_print(shell, "Invalid operation");
            return -1;
        }
    }
    return 0;
}

int cmd_servo_move(const struct shell *shell, size_t argc, char **argv) {
    int servoNum = 0;
    bool do_open = false;
    parse_servo_args(shell, argc, argv, &servoNum, &do_open);
    shell_print(shell, "Parsed: #%d doOpen: %d", servoNum, (int) do_open);
    change_servo(*(servos[servoNum]), do_open);

    return 0;
}

int cmd_servo_sweep(const struct shell *shell, size_t argc, char **argv) {
    int servoNum = 0;
    bool do_open = false;
    parse_servo_args(shell, argc, argv, &servoNum, &do_open);
    shell_print(shell, "Parsed: #%d doOpen: %d", servoNum, (int) do_open);
    sweep_servo(*(servos[servoNum]), do_open);

    return 0;
}
