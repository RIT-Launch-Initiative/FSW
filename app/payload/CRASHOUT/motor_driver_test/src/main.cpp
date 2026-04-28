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

/* Motor Driver Register Addresses */
#define RC_STATUS1    0x01
#define RC_STATUS2    0x02
#define RC_STATUS3    0x03
#define REG_STATUS1   0x04
#define REG_STATUS2   0x05
#define REG_STATUS3   0x06

#define CONFIG0_REG   0x09
#define CONFIG1_REG   0x0a
#define CONFIG2_REG   0x0b
#define CONFIG3_REG   0x0c
#define CONFIG4_REG   0x0d

#define REG_CTRL0_REG 0x0e
#define REG_CTRL1_REG 0x0f
#define REG_CTRL2_REG 0x10

#define RC_CTRL0_REG  0x11
#define RC_CTRL1_REG  0x12
#define RC_CTRL2_REG  0x13
#define RC_CTRL3_REG  0x14
#define RC_CTRL4_REG  0x15
#define RC_CTRL5_REG  0x16
#define RC_CTRL6_REG  0x17
#define RC_CTRL7_REG  0x18
#define RC_CTRL8_REG  0x19

class Motor {
    public:

    /* Global variables */
    struct i2c_dt_spec motor;
    const struct device *enc;
    uint8_t flt = 0;
    // w_scale_value is used to convert the speed value read from the motor driver in Rad/s to RPM
    // The default value 16 is the minimum value and maxes the motor out at 4080 Rad/s (Page 31 of datatsheet)
    #define DEFAULT_W_SCALE_VALUE 16
    uint8_t w_scale_value = DEFAULT_W_SCALE_VALUE;

    /**
     * Contructs an instance of the Motor class with the given i2c device specification.
     */
    Motor(struct i2c_dt_spec motor_spec){
        motor = motor_spec;
        enc = NULL;
    }

    /**
     * Contructs an instance of the Motor class with the given i2c device specification and encoder.
     */
    Motor(struct i2c_dt_spec motor_spec, const struct device *dcm_enc){
        motor = motor_spec;
        enc = dcm_enc;
    }

    /**
     * Sets the voltage of the motor driver by writing to the appropriate register.
     */
    void setVoltage(float volts) {
        if (volts > 38) {
            printk("invalid volts");
            return;
        }
        // The value 228 is the maximum value that can be written to the motor driver and corresponds to 38 volts (Page 45 of datasheet).
        float val = (volts / 38.f) * 227;
        uint8_t regval = (uint8_t) (val + .5f);
        i2c_reg_write_byte_dt(&motor, REG_CTRL1_REG, regval);
    }

    /**
     * Sets the w_scale value of the motor driver by writing to the appropriate register 
     * if w_scale given is valid (16, 32, 64, or 128).
     */
    void set_w_scale_value(uint32_t w_scale){
        uint8_t regval;
        i2c_reg_read_byte_dt(&motor, REG_CTRL0_REG, &regval);
        if (w_scale == 16){
            // set bits 0,1 to 00 to set w_scale to 16 (page 31 of datasheet)
            regval &= 0b11111100; 
            i2c_reg_write_byte_dt(&motor, REG_CTRL0_REG, regval);
        } else if (w_scale == 32){
            // set bits 0,1 to 01 to set w_scale to 32 (page 31 of datasheet)
            regval &= 0b11111101;
            regval |= 0b00000001;
            i2c_reg_write_byte_dt(&motor, REG_CTRL0_REG, regval);
        } else if (w_scale == 64){
            // set bits 0,1 to 10 to set w_scale to 64 (page 31 of datasheet)
            regval &= 0b11111110;
            regval |= 0b00000010;
            i2c_reg_write_byte_dt(&motor, REG_CTRL0_REG, regval);
        } else if (w_scale == 128){
            // set bits 0,1 to 11 to set w_scale to 128 (page 31 of datasheet)
            regval |= 0b00000011;
            i2c_reg_write_byte_dt(&motor, REG_CTRL0_REG, regval);
        } else {
            printk("invalid w_scale");
            return;
        }
        w_scale_value = w_scale;
    }

    /**
     * Sets the speed of the motor driver by writing to the appropriate register.
     * @param radps the speed to set the motor to in Rad/s
     */
    void setSpeed(uint16_t radps) {
        if (radps < 4080){
            set_w_scale_value(16);
        } else if (radps < 8160){
            set_w_scale_value(32);
        } else if (radps < 16320){
            set_w_scale_value(64);
        } else if (radps < 32640){
            set_w_scale_value(128);
        } else {
            return;
            printk("invalid speed");
        }
        uint16_t speed_val = radps / w_scale_value;
        i2c_reg_write_byte_dt(&motor, REG_CTRL1_REG, speed_val);
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
     * Sets the motor driver to speed control mode
     */
    void setToSpeedControlMode(){
        uint8_t reg_ctrl_0;
        i2c_reg_read_byte_dt(&motor, REG_CTRL0_REG, &reg_ctrl_0);

        // set bits 4,3 to 10 to set speed control mode (page 30 of datasheet)
        reg_ctrl_0 &= 0b11110111;
        reg_ctrl_0 |= 0b00010000;
        i2c_reg_write_byte_dt(&motor, REG_CTRL0_REG, reg_ctrl_0);

        // set duty ctrl to 0
        uint8_t config0;
        i2c_reg_read_byte_dt(&motor, CONFIG0_REG, &config0);
        config0 &= 0b11111110;
        i2c_reg_write_byte_dt(&motor, CONFIG0_REG, config0);
    }

    /**
     * Sets the motor driver to voltage control mode
     */
    void setToVoltageControlMode(){
        uint8_t reg_ctrl_0;
        i2c_reg_read_byte_dt(&motor, REG_CTRL0_REG, &reg_ctrl_0);

        // set bits 4,3 to 11 to set voltage control mode (page 30 of datasheet)
        reg_ctrl_0 |= 0b00011000;
        i2c_reg_write_byte_dt(&motor, REG_CTRL0_REG, reg_ctrl_0);

        // set duty ctrl to 0
        uint8_t config0;
        i2c_reg_read_byte_dt(&motor, CONFIG0_REG, &config0);
        config0 &= 0b11111110;
        i2c_reg_write_byte_dt(&motor, CONFIG0_REG, config0);
    }

    /**
     * Reads and prints the voltage, speed, ripple count, and direction from the motor driver.
     */
    void printInfo(){
        //Read voltage from the motor driver
        uint8_t vmtr = 0;
        i2c_reg_read_byte_dt(&motor, REG_STATUS1, &vmtr);
        float volts = map_voltage(vmtr);

        //Read speed from the motor driver
        uint8_t speed;
        i2c_reg_read_byte_dt(&motor, RC_STATUS1, &speed);
        printk("Speed: %02x\n", speed);
        float speedrpm = speed_rpm(speed, w_scale_value);

        //Read ripple count from the motor driver
        uint8_t rcc0 = 0;
        uint8_t rcc1 = 0;
        i2c_reg_read_byte_dt(&motor, RC_STATUS2, &rcc0);
        i2c_reg_read_byte_dt(&motor, RC_STATUS3, &rcc1);
        uint16_t rc_cnt = (rcc1 << 8) | rcc0;

        //Read direction from the motor driver
        uint8_t dir = 0;
        i2c_reg_read_byte_dt(&motor, CONFIG4_REG, &dir);

        //Print voltage, speed, ripple count, and direction returned by the motor driver
        printk("V=%.4f V, S=%.4f rpm, RC=%d, Dir=%02x ", (double) volts, (double) speedrpm, (int) rc_cnt, dir);
        if(dir == 0x36){
            printk("Forward\n");
        } else if(dir == 0x37){
            printk("Backward\n");
        } else if(dir == 0x34){
            printk("Braking\n");
        } else{
            printk("Failed to read Direction\n");
        }
    }

    /**
     * Reads and prints the fault register from the motor driver.
     */
    void printFault(){
        i2c_reg_read_byte_dt(&motor, 0, &flt);
        printk("Fault: %02x\n", flt);
    }

    /**
     * Prints the values of all of the registers in the motor driver
     */
    void regDump(){
        for(int i = 0; i <= 0x19; i++){
            i2c_reg_read_byte_dt(&motor, i, &flt);
            printk("Reg%02x: %02x\n", i, flt);
        }
    }

    /**
     * Sets up the ripple counting on the motor driver by writing to the appropriate registers.
     */
    void setupRippleCounting(){
        static const uint8_t inv_r_scale = 0b10;
        static const uint8_t inv_r = 42;

        static const uint8_t kmc_scale = 0b01;
        static const uint8_t kmc = 139;

        uint8_t rc_ctrl_2 = (inv_r_scale << 6) | (kmc_scale << 4) | (3 << 2) | 3;
        uint8_t rc_ctrl_3 = inv_r;
        uint8_t rc_ctrl_4 = kmc;

        i2c_reg_write_byte_dt(&motor, RC_CTRL0_REG, 0b11100010);
        i2c_reg_write_byte_dt(&motor, RC_CTRL2_REG, rc_ctrl_2);
        i2c_reg_write_byte_dt(&motor, RC_CTRL3_REG, rc_ctrl_3);
        i2c_reg_write_byte_dt(&motor, RC_CTRL4_REG, rc_ctrl_4);
    }

    /**
     * Enable overvoltage protection, and stall protection
     */
    void enableOVSProtection(){
        uint8_t regval;
        i2c_reg_read_byte_dt(&motor, CONFIG0_REG, &regval);
        regval |= 0b01100000;
        i2c_reg_write_byte_dt(&motor, CONFIG0_REG, regval);
    }

    /**
     * Disable overvoltage protection, and stall protection
     */
    void disableOVSProtection(){
        uint8_t regval;
        i2c_reg_read_byte_dt(&motor, CONFIG0_REG, &regval);
        regval &= 0b10011111;
        i2c_reg_write_byte_dt(&motor, CONFIG0_REG, regval);
    }

    /**
     * Enable motor spinning, overvoltage protection, and stall protection
     */
    void enableSpin(){
        uint8_t regval;
        i2c_reg_read_byte_dt(&motor, CONFIG0_REG, &regval);
        regval |= 0b10000000;
        i2c_reg_write_byte_dt(&motor, CONFIG0_REG, regval);
        enableOVSProtection();
    }

    /**
     * Disable motor spinning
     */
    void disableSpin(){
        uint8_t regval;
        i2c_reg_read_byte_dt(&motor, CONFIG0_REG, &regval);
        regval &= 0b01111111;
        i2c_reg_write_byte_dt(&motor, CONFIG0_REG, regval);
    }

    /**
     * Sets the spin mode of the motor driver by writing to the appropriate register.
     * @param mode: 0 = forward, 1 = backward, 2 = brake
     * We don't actually know which way forward and backward are, because it depends on how the motor is wired
     */
    void setSpinMode(uint8_t mode){
        if (mode == 0){
            i2c_reg_write_byte_dt(&motor, CONFIG4_REG, 0x36);
        } else if (mode == 1){
            i2c_reg_write_byte_dt(&motor, CONFIG4_REG, 0x37);
        } else if (mode == 2){
            i2c_reg_write_byte_dt(&motor, CONFIG4_REG, 0x34);
        }
    }

    /**
     * Enables the soft start feature of the motor driver by writing to the appropriate register.
     */
    void enableSoftStart(){
        uint8_t regval;
        i2c_reg_read_byte_dt(&motor, REG_CTRL0_REG, &regval);
        regval |= 0b00100000;
        i2c_reg_write_byte_dt(&motor, REG_CTRL0_REG, regval);
    }

    /**
     * Disables the soft start feature of the motor driver by writing to the appropriate register.
     */
    void disableSoftStart(){
        uint8_t regval;
        i2c_reg_read_byte_dt(&motor, REG_CTRL0_REG, &regval);
        regval &= 0b11011111;
        i2c_reg_write_byte_dt(&motor, REG_CTRL0_REG, regval);
    }

    /**
     * Tests running the motor by controlling the voltage sent to it
     * @param dir the direction to run the motor in, 0 for forward and 1 for backward
     */
    void voltageControlTest(uint8_t dir){
        setToVoltageControlMode();
        setupRippleCounting();

        setSpinMode(2); // brake mode to start
        printFault();
        
        setSpeed(0);

        enableSpin();
        printFault();

        setSpinMode(dir);
        printFault();

        for (int i = 4; i <= 12; i++){
            setVoltage(i);
            printInfo();
            k_msleep(500);
        }
        
        setSpinMode(2); // brake mode to stop
    }

    /**
     * Tests running the motor by directly controlling the speed
     * @param dir the direction to run the motor in, 0 for forward and 1 for backward
     */
    void speedControlTest(uint8_t dir){
        setupRippleCounting();
        setToSpeedControlMode();

        setSpinMode(2); // brake mode to start
        printFault();
        
        setSpeed(0);

        enableSpin();
        printFault();

        setSpinMode(dir);
        printFault();

        for (int i = 1; i <= 10; i++){
            setSpeed(i * 40);
            printInfo();
            k_msleep(2000);
        }

        setSpinMode(2); // brake mode to stop
    }

    /**
     * Sets up the motor driver to run in speed control mode
     */
    int initSpeedControl(){
        if (!device_is_ready(i2c_bus)) {
            printk("No i2c ready");
            return 0;
        }
        setupRippleCounting();
        setToSpeedControlMode();
        // for some reason I need to run this again after speed mode
        // Maybe an i2c failure? maybe breadboard noise? unsure, retest on crashout board
        setupRippleCounting(); 
        //regDump();
        enableSoftStart();
        setSpinMode(2); // default to brake mode
        setSpeed(0);
        return 1;
    }

    /**
     * Sets up the motor driver to run in voltage control mode
     */
    int initVoltageControl(){
        if (!device_is_ready(i2c_bus)) {
            printk("No i2c ready");
            return 0;
        }

        setToVoltageControlMode();
        setupRippleCounting();
        enableSoftStart();
        setSpinMode(2); // default to brake mode
        setVoltage(0);
        return 1;
    }

    /**
     * Reads the motor's quadrature encoder (if it has one)
     */
    int64_t read_enc(){
        if (enc == NULL) return 0xFFFFFFFFFFFFFFFF; // int 64 -limit

        struct sensor_value counter_val;

        int ret = sensor_sample_fetch(enc);
        if (ret < 0) {
            printk("Unable to fetch sensor sample: %d\n", ret);
            return ret;
        }

        ret = sensor_channel_get(enc, SENSOR_CHAN_ROTATION, &counter_val);
        if (ret < 0) {
            printk("Unable to read sensor channel: %d\n", ret);
            return ret;
        }
        int64_t millirotation = sensor_value_to_micro(&counter_val);

        return millirotation;
    }
};

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
    Motor motor1(motor1_i2c, yaw_enc);
    Motor motor2(motor2_i2c, pitch_enc);
    Motor motor3(motor3_i2c, dcm_enc3);

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
        for (int i = 90'000'000; i <= 270'000'000; i += 10'000'000){
            doPid(motor3, i);
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
