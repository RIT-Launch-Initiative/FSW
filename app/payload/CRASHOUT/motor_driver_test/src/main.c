/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

const struct device* const i2c_bus = DEVICE_DT_GET(DT_NODELABEL(arduino_i2c));
const struct i2c_dt_spec i2c_dev = {.bus = i2c_bus, .addr = 0x30};

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
// #define NSLEEP_NODE DT_ALIAS(nsleep)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

// static const struct gpio_dt_spec nSleep = GPIO_DT_SPEC_GET(NSLEEP_NODE, gpios);

float map_voltage(uint8_t vmtr) { return ((float) vmtr / 228.f) * 38.f; }

float speed_rpm(uint8_t speed, int32_t w_scale) {
    float radps = speed * w_scale;
    float rpm = 60.f * radps / 6.28318f;
    return rpm;
}

#define RC_STATUS1  0x1
#define RC_STATUS2  0x2
#define RC_STATUS3  0x3
#define REG_STATUS1 0x4
#define REG_STATUS2 0x5

#define CONFIG0_REG 0x9
#define CONFIG1_REG 0xa
#define CONFIG2_REG 0xb
#define CONFIG3_REG 0xc
#define CONFIG4_REG 0xd

#define REG_CTRL_0_REG 0xe
#define REG_CTRL_1_REG 0xf

#define RC_CTRL0_REG 0x11
#define RC_CTRL1_REG 0x12
#define RC_CTRL2_REG 0x13
#define RC_CTRL3_REG 0x14
#define RC_CTRL4_REG 0x15
#define RC_CTRL5_REG 0x16
#define RC_CTRL6_REG 0x17
#define RC_CTRL7_REG 0x18

void set_voltage(float volts) {
    if (volts > 38) {
        printk("invalid volts");
        return;
    }
    float val = (volts / 38.f) * 227;
    uint8_t regval = (uint8_t) (val + .5f);
    i2c_reg_write_byte_dt(&i2c_dev, REG_CTRL_1_REG, regval);
}

int main(void) {
    int ret;
    bool led_state = true;

    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }
    if (!device_is_ready(i2c_bus)) {
        printk("No i2c ready");
        return 0;
    }
    k_msleep(500);
    // clear all
    i2c_reg_write_byte_dt(&i2c_dev, CONFIG0_REG, 0b0110 | 0xc0);

    // resistance .001 kOhm
    // resistance 1ohm = R
    // inv_r_scale = 64 = 0b01
    // INV_R_SCALE / R = 64
    // INV_R = 64
    // RC_CONTROL_2 = 0x13
    static const uint8_t inv_r_scale = 0b01;
    static const uint8_t inv_r = 64;

    static const uint8_t kmc_scale = 0b01;
    static const uint8_t kmc = 139;

    uint8_t rc_ctrl_2 = (inv_r_scale << 6) | (kmc_scale << 4) | (3 << 2) | 3;
    uint8_t rc_ctrl_3 = inv_r;
    uint8_t rc_ctrl_4 = kmc;

    i2c_reg_write_byte_dt(&i2c_dev, RC_CTRL2_REG, rc_ctrl_2);
    i2c_reg_write_byte_dt(&i2c_dev, RC_CTRL3_REG, rc_ctrl_3);
    i2c_reg_write_byte_dt(&i2c_dev, RC_CTRL4_REG, rc_ctrl_4);

    // max speed = 1046.666666666667 rad/s at motor
    // uint8_t w_scale_bit = 0b00; // x16 max speed 4080 rad/s
    uint32_t w_scale_value = 16;
    uint8_t reg_ctrl_0 = 0b00111111; // defaults and x16 w_scale and voltage control
    i2c_reg_write_byte_dt(&i2c_dev, REG_CTRL_0_REG, reg_ctrl_0);

    printk("Initted i2c dev\n");

    uint8_t flt = 0;
    i2c_reg_write_byte_dt(&i2c_dev, CONFIG4_REG, 0x34); // 0x36 one dir, 0x37 other dir
    i2c_reg_read_byte_dt(&i2c_dev, 0, &flt);
    printk("Fault 34: %02x\n", flt);
    i2c_reg_write_byte_dt(&i2c_dev, CONFIG0_REG, 0xe0);
    i2c_reg_read_byte_dt(&i2c_dev, 0, &flt);
    printk("Fault e0: %02x\n", flt);
    i2c_reg_write_byte_dt(&i2c_dev, CONFIG4_REG, 0x36); // 0x36 one dir, 0x37 other dir
    i2c_reg_read_byte_dt(&i2c_dev, 0, &flt);
    printk("Fault 36: %02x\n", flt);

    i2c_reg_read_byte_dt(&i2c_dev, 0, &flt);
    printk("Fault: %02x\n", flt);

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }

    set_voltage(11.0);

	float weezer[10] = {9.6, 7.1, 9.6, 11.0, 11.9, 10.6, 9.6, 7.6, 7.1, 7.1};
    for (int i = 0; i < 100; i++) {
        float freq = weezer[(i/10) % 10];
		set_voltage(freq);
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            return 0;
        }

        led_state = !led_state;
        // printf("LED state: %s\n", led_state ? "ON" : "OFF");
        uint8_t vmtr = 0;
        i2c_reg_read_byte_dt(&i2c_dev, REG_STATUS1, &vmtr);
        float volts = map_voltage(vmtr);
        uint8_t speed = 0;
        i2c_reg_read_byte_dt(&i2c_dev, RC_STATUS1, &speed);
        float speedrpm = speed_rpm(speed, w_scale_value);

        uint8_t rcc0 = 0;
        uint8_t rcc1 = 0;
        i2c_reg_read_byte_dt(&i2c_dev, RC_STATUS2, &rcc0);
        i2c_reg_read_byte_dt(&i2c_dev, RC_STATUS3, &rcc1);
        uint16_t rc_cnt = (rcc1 << 8) | rcc0;

        printk("V=%.4f V, S=%.4f rpm, RC=%d\n", (double) volts, (double) speedrpm, (int) rc_cnt);

        i2c_reg_read_byte_dt(&i2c_dev, 0, &flt);

        k_msleep(40);
    }
    i2c_reg_write_byte_dt(&i2c_dev, CONFIG4_REG, 0x34); // 0x36 one dir, 0x37 other dir

    return 0;
}
