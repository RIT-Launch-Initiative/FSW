#include "servo.h"

#include <cmath>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(servo, CONFIG_APP_AIRBRAKE_LOG_LEVEL);

#ifdef CONFIG_PWM

#define SERVO_NODE DT_ALIAS(servo)
static const struct pwm_dt_spec servo = PWM_DT_SPEC_GET(SERVO_NODE);
static const uint32_t min_pulse = DT_PROP(SERVO_NODE, min_pulse);
static const uint32_t max_pulse = DT_PROP(SERVO_NODE, max_pulse);
#endif

#define SERVO_ENABLE_NODE DT_ALIAS(servo_en)
static const struct gpio_dt_spec servo_enable = GPIO_DT_SPEC_GET(SERVO_ENABLE_NODE, gpios);

extern "C" int servo_init() {
#ifdef CONFIG_PWM
    if (!pwm_is_ready_dt(&servo)) {
        LOG_ERR("Airbrake actuator device '%s' is not ready\n", servo.dev->name);
        return -1;
    }
#endif

    if (!gpio_is_ready_dt(&servo_enable)) {
        LOG_ERR("Airbrake actuator device enable pin is not ready");
        return -1;
    }
    int ret = gpio_pin_configure_dt(&servo_enable, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        LOG_ERR("Failed to configure airbrakes actuator enable pin");
        return ret;
    }

    return 0;
}

int EnableServo() { return gpio_pin_set_dt(&servo_enable, 1); }

int DisableServo() {
#ifdef CONFIG_PWM
    int ret = pwm_set_pulse_dt(&servo, 0);
    if (ret != 0) {
        LOG_WRN("Failed to disable servo command. Not disabling power");
        return ret;
    }
#endif
    return gpio_pin_set_dt(&servo_enable, 0);
}

int SetServoEffort(float effort) {
    if (std::isnan(effort) || std::isinf(effort) || effort < 0 || effort > 1) {
        LOG_WRN_ONCE("Invalid servo effort (want between 0 and 1)");
    }
#ifndef CONFIG_PWM
    LOG_INF("set to %f", (double) effort);
    return 0; // silly native sim
#else

    uint32_t pulse = (uint32_t) (effort * (max_pulse - min_pulse)) + min_pulse;
    return pwm_set_pulse_dt(&servo, pulse);
#endif
}
