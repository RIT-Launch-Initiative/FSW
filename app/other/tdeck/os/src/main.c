/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aw9364.h"

#include <lvgl.h>
#include <lvgl_input_device.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app);
// static struct gpio_dt_spec stuff_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(stuffenable), gpios);

const struct device *bl_dev = DEVICE_DT_GET(DT_NODELABEL(backlight));

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DEFAULT_RADIO_NODE), "No default LoRa radio specified in DT");
const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);

static uint32_t countx = 0;
static uint32_t county = 0;

int setup_lora() {
    struct lora_modem_config config;
    int ret;

    if (!device_is_ready(lora_dev)) {
        LOG_ERR("%s Device not ready", lora_dev->name);
        return -ENODEV;
    }

    config.frequency = 434000000;
    config.bandwidth = BW_125_KHZ;
    config.datarate = SF_10;
    config.preamble_len = 8;
    config.coding_rate = CR_4_5;
    config.iq_inverted = false;
    config.public_network = false;
    config.tx_power = 4;
    config.tx = true;

    ret = lora_config(lora_dev, &config);
    if (ret < 0) {
        LOG_ERR("LoRa config failed");
        return ret;
    }
    return 0;
}

static void callback_delta(lv_event_t *e) {
    ARG_UNUSED(e);

    int32_t delta = (int) lv_event_get_user_data(e);

    county = county + delta;
}

static void radio_callback(lv_event_t *e) {
    uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};
    int ret = lora_send(lora_dev, data, 8);
    if (ret < 0) {
        LOG_ERR("LoRa send failed");
        return;
    }
}

void input_cb(struct input_event *evt, void *user_data) {
    LOG_INF("Input callback");

    if (evt->code == INPUT_KEY_UP) {
        county = county + 1;
    } else if (evt->code == INPUT_KEY_DOWN) {
        county -= 1;
    } else if (evt->code == INPUT_KEY_LEFT) {
        countx -= 1;
    } else if (evt->code == INPUT_KEY_RIGHT) {
        countx += 1;
    }

    return;
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(buttons)), input_cb, 0);

int main(void) {
    char count_str[11] = {0};
    const struct device *display_dev;
    lv_obj_t *add1label;
    lv_obj_t *sub1label;
    lv_obj_t *radiolabel;
    lv_obj_t *count_labelx;
    lv_obj_t *count_labely;

    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Device not ready, aborting test");
        return 0;
    }
    printk("device ready\n");
    setup_lora();

    /*Change the active screen's background color*/
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(255, 255, 255), LV_PART_MAIN);

    lv_obj_t *add1button;
    lv_obj_t *sub1button;

    add1button = lv_button_create(lv_screen_active());
    lv_obj_align(add1button, LV_ALIGN_TOP_RIGHT, 0, 10);
    lv_obj_add_event_cb(add1button, callback_delta, LV_EVENT_CLICKED, (void *) 1);
    add1label = lv_label_create(add1button);

    sub1button = lv_button_create(lv_screen_active());
    lv_obj_align(sub1button, LV_ALIGN_TOP_LEFT, 0, 10);
    lv_obj_add_event_cb(sub1button, callback_delta, LV_EVENT_CLICKED, (void *) -1);
    sub1label = lv_label_create(sub1button);

    lv_label_set_text(add1label, "+1");
    lv_obj_align(add1label, LV_ALIGN_CENTER, 0, 0);

    lv_label_set_text(sub1label, "-1");
    lv_obj_align(sub1label, LV_ALIGN_CENTER, 0, 0);

    count_labelx = lv_label_create(lv_screen_active());
    lv_obj_align(count_labelx, LV_ALIGN_TOP_MID, 0, 20);

    count_labely = lv_label_create(lv_screen_active());
    lv_obj_align(count_labely, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_t *radiobutton;

    radiobutton = lv_button_create(lv_screen_active());
    lv_obj_align(radiobutton, LV_ALIGN_CENTER, 0, 10);
    lv_obj_add_event_cb(radiobutton, radio_callback, LV_EVENT_CLICKED, 0);
    radiolabel = lv_label_create(radiobutton);

    lv_label_set_text(radiolabel, "Beep!");
    lv_obj_align(radiolabel, LV_ALIGN_CENTER, 0, 0);

    lv_timer_handler();

    display_blanking_off(display_dev);

    int last_countx = ~0;
    int last_county = ~0;
    while (1) {
        if (countx != last_countx) {
            sprintf(count_str, "x: %d", countx);
            lv_label_set_text(count_labelx, count_str);
            last_countx = countx;
        }

        if (county != last_county) {
            sprintf(count_str, "y: %d", county);
            lv_label_set_text(count_labely, count_str);
            last_county = county;
        }

        lv_timer_handler();
        k_sleep(K_MSEC(10));
    }
}
