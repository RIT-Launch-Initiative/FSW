#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

// flip
static const struct pwm_dt_spec servo1 = PWM_DT_SPEC_GET(DT_ALIAS(servo1));
// not flipped
static const struct pwm_dt_spec servo2 = PWM_DT_SPEC_GET(DT_ALIAS(servo2));
static const struct pwm_dt_spec servo3 = PWM_DT_SPEC_GET(DT_ALIAS(servo3));
const pwm_dt_spec *servos[3] = {&servo1, &servo2, &servo3};

static const uint32_t min_pulse = PWM_USEC(800);  //DT_PROP(DT_PARENT(DT_A LIAS(servo1)), min_pulse);
static const uint32_t max_pulse = PWM_USEC(1700); //DT_PROP(DT_PARENT(DT_ALIAS(servo1)), max_pulse);

// false when closed, true when open
bool state1 = 0;
bool state2 = 0;
bool state3 = 0;
bool *states[] = {&state1, &state2, &state3};

// close open
using PulseDefs = uint32_t[2];
PulseDefs pulse1 = {PWM_USEC(2200), PWM_USEC(800)};
PulseDefs pulse2 = {min_pulse, max_pulse};
PulseDefs pulse3 = {min_pulse, max_pulse};

PulseDefs *pulsedefs[] = {&pulse1, &pulse2, &pulse3};

void change_servo(int idx, bool newstate) {
    *states[idx] = newstate;
    PulseDefs *ps = pulsedefs[idx];
    uint32_t pulse = (*ps)[newstate];
    auto servo = servos[idx];

    int ret = pwm_set_pulse_dt(servo, pulse);
    if (ret < 0) {
        printk("Error %d: failed to set pulse width\n", ret);
        return;
    }
}

// 1000 2000 100deg
//  800 2200 145deg no jitter
//  600 2400 145deg no jitter

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
    for (int i = 0; i < 10; i++) {
        change_servo(0, false);
        k_msleep(1000);
        change_servo(1, false);
        k_msleep(1000);
        change_servo(2, false);
        k_msleep(1000);

        change_servo(0, true);
        k_msleep(1000);
        change_servo(1, true);
        k_msleep(1000);
        change_servo(2, true);
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

int cmd_servo(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(shell);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    if (argc != 2 && argc != 3) {
        shell_print(shell, "Wrong number of args");
        shell_print(shell, "Help: servo 1/2/3 o/c");
        return -1;
    }
    char servoNumChar = argv[1][0];

    int servoNum = 0;
    if (servoNumChar == '1') {
        servoNum = 0;
    } else if (servoNumChar == '2') {
        servoNum = 1;
    } else if (servoNumChar == '3') {
        servoNum = 2;
    } else {
        shell_print(shell, "Invalid servo number");
        return -1;
    }
    if (argc == 2) {
        bool do_open = !(*states[servoNum]);
        change_servo(servoNum, do_open);
        shell_print(shell, "Parsed: #%d doOpen: %d", servoNum, (int) do_open);

    } else if (argc == 3) {

        char operationChar = argv[2][0];
        shell_print(shell, "Servo Control %c %c", servoNumChar, operationChar);
        bool do_open = true;

        if (operationChar == 'o') {
            do_open = true;
        } else if (operationChar == 'c') {
            do_open = false;
        } else {
            shell_print(shell, "Invalid operation");
            return -1;
        }
        shell_print(shell, "Parsed: #%d doOpen: %d", servoNum, (int) do_open);
        change_servo(servoNum, do_open);
    }

    return 0;
}
