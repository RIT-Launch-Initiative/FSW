#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>

static const struct pwm_dt_spec servo1 = PWM_DT_SPEC_GET(DT_ALIAS(servo1));
static const struct pwm_dt_spec servo2 = PWM_DT_SPEC_GET(DT_ALIAS(servo2));
static const struct pwm_dt_spec servo3 = PWM_DT_SPEC_GET(DT_ALIAS(servo3));

static const uint32_t min_pulse = PWM_USEC(1000); //DT_PROP(DT_PARENT(DT_A LIAS(servo1)), min_pulse);
static const uint32_t max_pulse = PWM_USEC(2000); //DT_PROP(DT_PARENT(DT_ALIAS(servo1)), max_pulse);

#define SERVO_EN DT_NODELABEL(servo_enable)
static const struct gpio_dt_spec servo_en = GPIO_DT_SPEC_GET(SERVO_EN, gpios);

#define LDO5V_EN DT_NODELABEL(ldo5v_enable)
static const struct gpio_dt_spec ldo5v_en = GPIO_DT_SPEC_GET(LDO5V_EN, gpios);

#define GPSRST_NODE DT_ALIAS(gpsreset)
static const struct gpio_dt_spec gpsreset = GPIO_DT_SPEC_GET(GPSRST_NODE, gpios);

#define GPSSAFE_NODE DT_ALIAS(gpssafeboot)
static const struct gpio_dt_spec gpstimepulse = GPIO_DT_SPEC_GET(GPSSAFE_NODE, gpios);

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

int reset_gps() {
    int ret;
    if (!gpio_is_ready_dt(&gpsreset)) {
        printk("No GPS RST :(\n");
        return 0;
    }
    if (!gpio_is_ready_dt(&gpstimepulse)) {
        printk("No GPS safe :(\n");
        return 0;
    }

    ret = gpio_pin_configure_dt(&gpsreset, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Failed to conf gps reset :(\n");
        return 0;
    }

    // Safeboot active low (send downwards before reset to enter safeboot)
    ret = gpio_pin_configure_dt(&gpstimepulse, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Failed to conf gps safe :(\n");
        return 0;
    }
    // Don't enter safeboot: pin to logic 0
    ret = gpio_pin_set_dt(&gpstimepulse, 0);
    if (ret < 0) {
        printk("couldnt set gpstimepulse: %d", ret);
    }

    k_msleep(1);

    // Gps Reset Routine

    ret = gpio_pin_set_dt(&gpsreset, 1);
    if (ret < 0) {
        printk("couldnt set gpsreset: %d", ret);
    }
    k_msleep(2);
    ret = gpio_pin_set_dt(&gpsreset, 0);
    if (ret < 0) {
        printk("couldnt set gpsreset: %d", ret);
    }

    ret = gpio_pin_configure_dt(&gpstimepulse, GPIO_INPUT);
    if (ret < 0) {
        printk("Failed to conf gps timepulse pin back to input:(\n");
        return 0;
    }
    printk("GPS Reset\n");
    return 0;
}
int main() {
    reset_gps();
    init_servo();

    uint32_t pulse_width = min_pulse;
    int ret;
    const pwm_dt_spec *servos[3] = {&servo1, &servo2, &servo3};

    printk("Servomotor control\n");

    for (int i = 0; i < 3; i++) {
        if (!pwm_is_ready_dt(servos[i])) {
            printk("Error: PWM device %s is not ready\n", servos[i]->dev->name);
            return 0;
        }
    }

    while (1) {
        for (int i = 0; i < 3; i++) {
            ret = pwm_set_pulse_dt(servos[i], pulse_width);
            if (ret < 0) {
                printk("Error %d: failed to set pulse width\n", ret);
                return 0;
            }
        }
        if (pulse_width == min_pulse) {
            pulse_width = max_pulse;
        } else {
            pulse_width = min_pulse;
        }
        k_sleep(K_SECONDS(1));
    }
    return 0;
}
