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
#include <string.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* The devicetree node identifiers*/
#define NSLEEP_NODE DT_NODELABEL(nsleep)
const struct device* const i2c_bus = DEVICE_DT_GET(DT_NODELABEL(arduino_i2c));
const struct i2c_dt_spec i2c_dev = {.bus = i2c_bus, .addr = 0x30};
/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec nSleep = GPIO_DT_SPEC_GET(NSLEEP_NODE, gpios);

/* Motor Driver Register Addresses */
#define RC_STATUS1  0x1
#define RC_STATUS2  0x2
#define RC_STATUS3  0x3
#define REG_STATUS1 0x4
#define REG_STATUS2 0x5
#define REG_STATUS3 0x6

#define CONFIG0_REG 0x9
#define CONFIG1_REG 0xa
#define CONFIG2_REG 0xb
#define CONFIG3_REG 0xc
#define CONFIG4_REG 0xd

#define REG_CTRL0_REG 0xe
#define REG_CTRL1_REG 0xf
#define REG_CTRL2_REG 0x10

#define RC_CTRL0_REG 0x11
#define RC_CTRL1_REG 0x12
#define RC_CTRL2_REG 0x13
#define RC_CTRL3_REG 0x14
#define RC_CTRL4_REG 0x15
#define RC_CTRL5_REG 0x16
#define RC_CTRL6_REG 0x17
#define RC_CTRL7_REG 0x18
#define RC_CTRL8_REG 0x19

/* Global variables */
uint8_t flt = 0;
bool motorOn = false;
// w_scale_value is used to convert the speed value read from the motor driver in Rad/s to RPM
// The value 16 is the minimum value and maxes the motor out at 4080 Rad/s (Page 31 of datatsheet)
uint32_t w_scale_value = 16;

/**
 * Sets the voltage of the motor driver by writing to the appropriate register.
 */
void set_voltage(float volts) {
    if (volts > 38) {
        printk("invalid volts");
        return;
    }
    // The value 228 is the maximum value that can be written to the motor driver and corresponds to 38 volts (Page 45 of datasheet).
    float val = (volts / 38.f) * 227;
    uint8_t regval = (uint8_t) (val + .5f);
    i2c_reg_write_byte_dt(&i2c_dev, REG_CTRL1_REG, regval);
}

/**
 * Converts the voltage value read from the motor driver to volts.
 * The value 228 is the maximum value that can be read from the motor driver and corresponds to 38 volts (Page 45 of datasheet).
 */
float map_voltage(uint8_t vmtr) { return ((float) vmtr / 228.f) * 38.f; }

/**
 * Converts the speed value read from the motor driver in Rad/s to RPM.
 */
float speed_rpm(uint8_t speed, int32_t w_scale) {
    float radps = speed * w_scale;
    float rpm = 60.f * radps / 6.28318f;
    return rpm;
}

/**
 * Turns on the motor driver by writing to the appropriate registers.
 */
void turnOn(){
    //reset flt variable to 0
    flt = 0;

    gpio_pin_configure_dt(&nSleep, GPIO_OUTPUT_INACTIVE);

    k_msleep(10);

    gpio_pin_configure_dt(&nSleep, GPIO_OUTPUT_ACTIVE);

    uint8_t rc_ctrl0_reg = 0b11100010;
    uint8_t reg_ctrl_0 = 0b00110111;
    i2c_reg_write_byte_dt(&i2c_dev, RC_CTRL0_REG, rc_ctrl0_reg);
    i2c_reg_write_byte_dt(&i2c_dev, REG_CTRL0_REG, reg_ctrl_0);
    i2c_reg_write_byte_dt(&i2c_dev, REG_CTRL1_REG, 0x03);
    motorOn = true;
}

/**
 * Clears the ripple count on the motor driver by resetting the nSleep pin and writing to the appropriate registers.
 */
void clearRippleCount(){
    // turn nSleep off then on again
    gpio_pin_configure_dt(&nSleep, GPIO_OUTPUT_INACTIVE);
    k_msleep(10);
    gpio_pin_configure_dt(&nSleep, GPIO_OUTPUT_ACTIVE);
    // reset registers to default values
    i2c_reg_write_byte_dt(&i2c_dev, RC_CTRL0_REG, 0b11100010);
    i2c_reg_write_byte_dt(&i2c_dev, REG_CTRL0_REG, 0b00100111);
    i2c_reg_write_byte_dt(&i2c_dev, REG_CTRL1_REG, 0x03); 
}

/**
 * Reads and prints the voltage, speed, ripple count, and direction from the motor driver.
 */
void print_info(){
    //Read voltage from the motor driver
    uint8_t vmtr = 0;
    i2c_reg_read_byte_dt(&i2c_dev, REG_STATUS1, &vmtr);
    float volts = map_voltage(vmtr);

    //Read speed from the motor driver
    uint8_t speed;
    i2c_reg_read_byte_dt(&i2c_dev, RC_STATUS1, &speed);
    printk("Speed: %02x\n", speed);
    float speedrpm = speed_rpm(speed, w_scale_value);

    //Read ripple count from the motor driver
    uint8_t rcc0 = 0;
    uint8_t rcc1 = 0;
    i2c_reg_read_byte_dt(&i2c_dev, RC_STATUS2, &rcc0);
    i2c_reg_read_byte_dt(&i2c_dev, RC_STATUS3, &rcc1);
    uint16_t rc_cnt = (rcc1 << 8) | rcc0;

    //Read direction from the motor driver
    uint8_t dir = 0;
    i2c_reg_read_byte_dt(&i2c_dev, CONFIG4_REG, &dir);

    //Print voltage, speed, ripple count, and direction returned by the motor driver
    printk("V=%.4f V, S=%.4f rpm, RC=%d, ", (double) volts, (double) speedrpm, (int) rc_cnt);
    if(dir == 0x36){
        printk("Clockwise\n");
    } else if(dir == 0x37){
        printk("Counter Clockwise\n");
    } else{
        printk("Failed to read Direction\n");
    }
}

/**
 * Tests running the motor by controlling the voltage sent to it
 */
void voltage_control_test(uint8_t dir){
    if(!motorOn){
        printk("Motor is off, cannot spin\n");
        return;
    }

    printk("Initted i2c dev\n");
    i2c_reg_write_byte_dt(&i2c_dev, CONFIG4_REG, 0x34); // 0011, 0100 
    i2c_reg_read_byte_dt(&i2c_dev, 0, &flt);
    printk("Fault 34: %02x\n", flt);
    
    i2c_reg_write_byte_dt(&i2c_dev, CONFIG0_REG, 0xe0);
    i2c_reg_read_byte_dt(&i2c_dev, 0, &flt);
    printk("Fault e0: %02x\n", flt);

    i2c_reg_write_byte_dt(&i2c_dev, CONFIG4_REG, dir); // 0x36 one dir, 0x37 other dir
    i2c_reg_read_byte_dt(&i2c_dev, 0, &flt);
    printk("Fault 36: %02x\n", flt);

    i2c_reg_read_byte_dt(&i2c_dev, 0, &flt);
    printk("Fault: %02x\n", flt);

    for (int i = 0; i < 100; i++){
        print_info();
        i2c_reg_read_byte_dt(&i2c_dev, 0, &flt);
        k_msleep(40);
    }
    i2c_reg_write_byte_dt(&i2c_dev, CONFIG4_REG, 0x34);
}

int main(void) {
    if (!device_is_ready(i2c_bus)) {
        printk("No i2c ready");
        return 0;
    }
    
    k_msleep(1000);
    turnOn();
    
    static const uint8_t inv_r_scale = 0b10;
    static const uint8_t inv_r = 42;

    static const uint8_t kmc_scale = 0b01;
    static const uint8_t kmc = 139;

    uint8_t rc_ctrl_2 = (inv_r_scale << 6) | (kmc_scale << 4) | (3 << 2) | 3;
    uint8_t rc_ctrl_3 = inv_r;
    uint8_t rc_ctrl_4 = kmc;

    i2c_reg_write_byte_dt(&i2c_dev, RC_CTRL2_REG, rc_ctrl_2);
    i2c_reg_write_byte_dt(&i2c_dev, RC_CTRL3_REG, rc_ctrl_3);
    i2c_reg_write_byte_dt(&i2c_dev, RC_CTRL4_REG, rc_ctrl_4);

    voltage_control_test(0x37);

    return 0;
}
