/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lvgl.h>
#include <lvgl_input_device.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

static struct gpio_dt_spec bl_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static struct gpio_dt_spec stuff_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(stuffenable), gpios);

static uint32_t count;

// LilyGo  T-Deck  control backlight chip has 16 levels of adjustment range
// The adjustable range is 0~15, 0 is the minimum brightness, 15 is the maximum
// brightness
void setBrightness(uint8_t value) {
    static uint8_t level = 0;
    static uint8_t steps = 16;
    if (value == 0) {
        gpio_pin_set_dt(&bl_gpio, 0);
        // delay(3);
        k_msleep(3);
        level = 0;
        return;
    }
    if (level == 0) {
        gpio_pin_set_dt(&bl_gpio, 1);
        level = steps;
        // delayMicroseconds(30);
        k_usleep(30);
    }
    int from = steps - level;
    int to = steps - value;
    int num = (steps + to - from) % steps;
    for (int i = 0; i < num; i++) {
        gpio_pin_set_dt(&bl_gpio, 0);
        k_usleep(1);
        gpio_pin_set_dt(&bl_gpio, 1);
        // digitalWrite(BOARD_BL_PIN, 0);
        // digitalWrite(BOARD_BL_PIN, 1);
    }
    level = value;
}
int main(void) {
    for (int i = 0; i < 50; i++) {
        printk("ASDASDASDA\n");
        k_msleep(10);
    }
    char count_str[11] = {0};
    const struct device *display_dev;
    lv_obj_t *hello_world_label;
    lv_obj_t *count_label;

    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Device not ready, aborting test");
        return 0;
    }
    printk("device ready\n");
    k_msleep(10);

    if (gpio_is_ready_dt(&bl_gpio)) {
        int err;
        printk("bl Is ready\n");
        err = gpio_pin_configure_dt(&bl_gpio, GPIO_OUTPUT);
        if (err) {
            printk("failed to configure backlight gpio: %d", err);
            return 0;
        }
        printk("bl confirured");
    } else {
        printk(" bl gpio not ready\n");
    }

    if (gpio_is_ready_dt(&stuff_gpio)) {
        int err;
        printk("stuff Is ready\n");
        err = gpio_pin_configure_dt(&stuff_gpio, GPIO_OUTPUT);
        if (err) {
            printk("failed to configure stuff gpio: %d", err);
            return 0;
        }
        printk("stuff confirured\n");

    } else {
        printk("stuff gpio not ready\n");
    }
    int gerr = gpio_pin_set_dt(&stuff_gpio, 1);
    if (gerr != 0) {
        printk("couldnt set stuff gpuio\n");
    }
    setBrightness(15);

    /*Change the active screen's background color*/
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(255, 255, 255), LV_PART_MAIN);

    hello_world_label = lv_label_create(lv_screen_active());

    printk("saying hello workld\n");
    lv_label_set_text(hello_world_label, "Hello world<");
    lv_obj_align(hello_world_label, LV_ALIGN_CENTER, 0, 0);

    printk("saying count\n");
    count_label = lv_label_create(lv_screen_active());
    lv_obj_align(count_label, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_timer_handler();
    display_blanking_off(display_dev);

    while (1) {
        if ((count % 100) == 0U) {
            sprintf(count_str, "%d", count / 100U);
            lv_label_set_text(count_label, count_str);
        }
        lv_timer_handler();
        ++count;
        k_sleep(K_MSEC(10));
    }
}
