#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define LDO_EN_NODE DT_NODELABEL(ldo_enable)
#define CAM_EN_NODE DT_NODELABEL(cam_enable)

const struct gpio_dt_spec ldo_enable = GPIO_DT_SPEC_GET(LDO_EN_NODE, gpios);
static const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(DT_ALIAS(buzzer), gpios);
static const struct gpio_dt_spec ematch = GPIO_DT_SPEC_GET(CAM_EN_NODE, gpios);

void set_buzz(int which) {
    gpio_pin_set_dt(&ldo_enable, which);
    gpio_pin_set_dt(&buzzer, which);
}

void set_ldo(int level) {
    gpio_pin_set_dt(&ldo_enable, level);
}

void set_ematch(int level) {
    gpio_pin_set_dt(&ematch, level);
}


void buzzer_init() {
    int ret = gpio_pin_configure_dt(&buzzer, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("Failed to conf buzzer pin:(");
        return;
    }
    ret = gpio_pin_configure_dt(&ldo_enable, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("Failed to conf ldo enable pin:(");
        return;
    }
}


void beep_full() {
    for (int i = 0; i < 10; i++) {
        printk("BEEP");
        set_buzz(1);
        k_msleep(1000);
        set_buzz(0);
        k_msleep(1000);
    }
}

void test_start_beep() {
    for (int i = 0; i < 3; i++) {
        printk("BEEP");
        set_buzz(1);
        k_msleep(100);
        set_buzz(0);
        k_msleep(100);
    }
}
void test_end_beep() {
    printk("BEEP");
    set_buzz(1);
    k_msleep(100);
    set_buzz(0);
    k_msleep(100);
    set_buzz(1);
    k_msleep(1000);
    set_buzz(0);
}