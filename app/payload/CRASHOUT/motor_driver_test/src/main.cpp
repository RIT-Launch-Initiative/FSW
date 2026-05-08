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
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <string.h>
#include "motor.cpp"

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* The devicetree node identifiers*/
#define NSLEEP_NODE DT_NODELABEL(nsleep)
const struct device* const i2c_bus = DEVICE_DT_GET(DT_NODELABEL(motor_i2c));
const struct i2c_dt_spec motor1_i2c = {.bus = i2c_bus, .addr = 0x30};
const struct i2c_dt_spec motor2_i2c = {.bus = i2c_bus, .addr = 0x32};
const struct i2c_dt_spec motor3_i2c = {.bus = i2c_bus, .addr = 0x36};

const struct device *yaw_enc = DEVICE_DT_GET(DT_NODELABEL(yaw_enc));
const struct device *pitch_enc = DEVICE_DT_GET(DT_NODELABEL(pitch_enc));
const struct device *dcm_enc3 = DEVICE_DT_GET(DT_NODELABEL(dcm_enc3));

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec nSleep = GPIO_DT_SPEC_GET(NSLEEP_NODE, gpios);


/**
* Resets the motor drivers by toggling the nsleep pin
* ***WARNING*** RESETS ALL MOTORS, WILL NEED TO REINIT AFTER CALLING THIS FUNCTION
*/
void reset(){
    gpio_pin_configure_dt(&nSleep, GPIO_OUTPUT_INACTIVE);
    k_msleep(10);
    gpio_pin_configure_dt(&nSleep, GPIO_OUTPUT_ACTIVE);
}


void doPid(Motor &mot, int64_t target){
    mot.enableSpin();
    int64_t integral = 0;
    int count = 0;
    for (int i = 0; i <= 200; i++){
        int64_t point = mot.read_enc();
        int64_t err = target - point;
        printk("%lld\n", point);

        if (err < 500000 && err > -500000) {
            count++;
        } else {
            count = 0;
        }

        if (count > 25){
            break;
        }

        int64_t kP = 9'000;
        int64_t kI = 0x7fffffffffffffff;
        int64_t outp = (err / kP) + (integral / kI);
        outp = outp / 11; // scale down output so we're not always running max speed
        // printk("%lld\n", outp);

        int dir = outp > 0 ? 0 : 1;
        int speed = (outp < 0 ? -outp : outp);
        if (speed > 32000){
            speed = 32000;
        } else {
            integral += err;
        }  
        // printk("Err: %lld %d %d\n", err/1000, dir, speed);
        // printk("m2: "); mot.printInfo();
        mot.setSpinMode(dir); // set motor 1 to forward
        mot.setSpeed(speed);
        k_msleep(10);
    }
    mot.disableSpin();
}

int main(void) { 
    Motor motor1(i2c_bus, motor1_i2c, yaw_enc);
    Motor motor2(i2c_bus, motor2_i2c, pitch_enc);
    Motor motor3(i2c_bus, motor3_i2c, dcm_enc3);

    reset();

    if (!motor1.initSpeedControl()){
        printk("Failed to initialize motor 1");
        return 0;
    }
    
    if (!motor2.initSpeedControl()){
        printk("Failed to initialize motor 2");
        return 0;
    }

    if (!motor3.initSpeedControl()){
        printk("Failed to initialize motor 3");
        return 0;
    }

    k_msleep(1000);

    while(1){  
        for (int i = 270'000'000; i >= 90'000'000; i -= 10'000'000){
            doPid(motor1, i);
            printk("m1: %lld\n", motor1.read_enc());
            k_msleep(500);
            doPid(motor2, i);
            printk("m2: %lld\n", motor2.read_enc());
            k_msleep(500);
            doPid(motor3, i);
            printk("m3: %lld\n", motor3.read_enc());
            k_msleep(500);
        }
    }

    // for (;;){
    //     int64_t md1 = motor1.read_enc();
    //     int64_t md2 = motor2.read_enc();
    //     int64_t md3 = motor3.read_enc();
    //     printk("Milldeg: %lld, %lld, %lld\n", md1/1000000, md2/1000000, md3/1000000);
    //     printk("m1: "); motor1.printInfo();
    //     printk("m2: "); motor2.printInfo();
    //     printk("m3: "); motor3.printInfo();
    //     k_msleep(200);
    // }
}
