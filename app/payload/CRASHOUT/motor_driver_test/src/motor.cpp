/*
 * @file motor.cpp
 * 
 * Motor Class for controlling the motor driver of a single motor
 * Contains methods for enabling and disabling various settings of the motor driver
 * Can control the motor in both voltage control mode and speed control mode
 *
 * @author Collin Casey, Richard Sommers
 */
#include "motor.hpp"

#include <zephyr/drivers/sensor.h>
/* Motor Driver Register Addresses */
static constexpr uint8_t REG_FAULT = 0x0;
static constexpr uint8_t RC_STATUS1 = 0x01;
static constexpr uint8_t RC_STATUS2 = 0x02;
static constexpr uint8_t RC_STATUS3 = 0x03;
static constexpr uint8_t REG_STATUS1 = 0x04;
static constexpr uint8_t REG_STATUS2 = 0x05;
static constexpr uint8_t REG_STATUS3 = 0x06;

static constexpr uint8_t CONFIG0_REG = 0x09;
static constexpr uint8_t CONFIG1_REG = 0x0a;
static constexpr uint8_t CONFIG2_REG = 0x0b;
static constexpr uint8_t CONFIG3_REG = 0x0c;
static constexpr uint8_t CONFIG4_REG = 0x0d;

static constexpr uint8_t REG_CTRL0_REG = 0x0e;
static constexpr uint8_t REG_CTRL1_REG = 0x0f;
static constexpr uint8_t REG_CTRL2_REG = 0x10;

static constexpr uint8_t RC_CTRL0_REG = 0x11;
static constexpr uint8_t RC_CTRL1_REG = 0x12;
static constexpr uint8_t RC_CTRL2_REG = 0x13;
static constexpr uint8_t RC_CTRL3_REG = 0x14;
static constexpr uint8_t RC_CTRL4_REG = 0x15;
static constexpr uint8_t RC_CTRL5_REG = 0x16;
static constexpr uint8_t RC_CTRL6_REG = 0x17;
static constexpr uint8_t RC_CTRL7_REG = 0x18;
static constexpr uint8_t RC_CTRL8_REG = 0x19;

/**
 * Contructs an instance of the Motor class with the given i2c address.
 */
Motor::Motor(const struct device* i2c_bus, uint8_t addr, bool flip_voltage)
    : Motor(i2c_bus, {.bus = i2c_bus, .addr = addr}, flip_voltage) {}

/**
 * Contructs an instance of the Motor class with the given i2c device specification.
 */
Motor::Motor(const struct device* i2c_bus, struct i2c_dt_spec motor_spec, bool flip_voltage_) {
    this->i2c_bus = i2c_bus;
    motor = motor_spec;
    enc = NULL;
    flip_voltage = flip_voltage_;
}

/**
 * Contructs an instance of the Motor class with the given i2c address and encoder.
 */
Motor::Motor(const struct device* i2c_bus, uint8_t addr, bool flip_voltage, const struct device* dcm_enc, bool flip_enc)
    : Motor(i2c_bus, {.bus = i2c_bus, .addr = addr}, flip_voltage, dcm_enc, flip_enc) {}

/**
 * Contructs an instance of the Motor class with the given i2c device specification and encoder.
 */
Motor::Motor(const struct device* i2c_bus, struct i2c_dt_spec motor_spec, bool flip_voltage_,
             const struct device* dcm_enc, bool flip_enc) {
    this->i2c_bus = i2c_bus;
    motor = motor_spec;
    enc = dcm_enc;
    flip_voltage = flip_voltage_;
    flip_encoder = flip_enc;
}

/**
 * Sets the voltage of the motor driver by writing to the appropriate register.
 */
void Motor::setVoltage(float volts) {
    if (volts > 38) {
        printk("invalid volts");
        return;
    }
    // The value 228 is the maximum value that can be written to the motor driver and corresponds to 38 volts (Page 45 of datasheet).
    float val = (volts / 38.f) * 227;
    uint8_t regval = (uint8_t) (val + .5f);
    i2c_reg_write_byte_dt(&motor, REG_CTRL1_REG, regval);
}
void Motor::setVoltage16(uint16_t millivolts) {
    if (millivolts > 38000) {
        printk("invalid volts");
        return;
    }
    // The value 228 is the maximum value that can be written to the motor driver and corresponds to 38 volts (Page 45 of datasheet).
    int val = ((uint32_t) millivolts * 228) / 38000;
    if (val > 228) {
        val = 228;
    }
    uint8_t regval = (uint8_t) val;
    i2c_reg_write_byte_dt(&motor, REG_CTRL1_REG, regval);
}

/**
 * Sets the w_scale value of the motor driver by writing to the appropriate register 
 * if w_scale given is valid (16, 32, 64, or 128).
 */
void Motor::set_w_scale_value(uint32_t w_scale) {
    uint8_t regval;
    i2c_reg_read_byte_dt(&motor, REG_CTRL0_REG, &regval);
    if (w_scale == 16) {
        // set bits 0,1 to 00 to set w_scale to 16 (page 31 of datasheet)
        regval &= 0b11111100;
        i2c_reg_write_byte_dt(&motor, REG_CTRL0_REG, regval);
    } else if (w_scale == 32) {
        // set bits 0,1 to 01 to set w_scale to 32 (page 31 of datasheet)
        regval &= 0b11111101;
        regval |= 0b00000001;
        i2c_reg_write_byte_dt(&motor, REG_CTRL0_REG, regval);
    } else if (w_scale == 64) {
        // set bits 0,1 to 10 to set w_scale to 64 (page 31 of datasheet)
        regval &= 0b11111110;
        regval |= 0b00000010;
        i2c_reg_write_byte_dt(&motor, REG_CTRL0_REG, regval);
    } else if (w_scale == 128) {
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
void Motor::setSpeed(uint16_t radps) {
    if (radps < 4080) {
        set_w_scale_value(16);
    } else if (radps < 8160) {
        set_w_scale_value(32);
    } else if (radps < 16320) {
        set_w_scale_value(64);
    } else if (radps < 32640) {
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
float Motor::map_voltage(uint8_t vmtr) { return ((float) vmtr / 228.f) * 38.f; }

/**
 * Converts the speed value read from the motor driver in Rad/s to RPM.
 */
float Motor::speed_rpm(uint8_t speed, int32_t w_scale) {
    float radps = speed * w_scale;
    float rpm = 60.f * radps / 6.28318f;
    return rpm;
}

/**
 * Sets the motor driver to speed control mode
 */
void Motor::setToSpeedControlMode() {
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
void Motor::setToVoltageControlMode() {
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
void Motor::printInfo() {
    //Read voltage from the motor driver
    uint8_t vmtr = 0;
    i2c_reg_read_byte_dt(&motor, REG_STATUS1, &vmtr);
    float volts = map_voltage(vmtr);

    uint8_t imtr = 0;
    i2c_reg_read_byte_dt(&motor, REG_STATUS2, &imtr);
    // 4 amps = 0xc0
    float amps = 1.0 * (imtr / 192.0);

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
    printk("V=%d mV, I=%d mA, S=%d rpm, RC=%d, Dir=%02x ", (int) (1000 * volts), (int) (1000 * amps),
           (int) (1000 * speedrpm), (int) rc_cnt, dir);
    if (dir == 0x36) {
        printk("Forward\n");
    } else if (dir == 0x37) {
        printk("Backward\n");
    } else if (dir == 0x34) {
        printk("Braking\n");
    } else {
        printk("Failed to read Direction\n");
    }
}

/**
 * Reads and prints the fault register from the motor driver.
 */
void Motor::printFault() {
    i2c_reg_read_byte_dt(&motor, 0, &flt);
    printk("Fault: %02x\n", flt);
}

void Motor::clearFault() {
    uint8_t reg = 0;
    i2c_reg_read_byte_dt(&motor, CONFIG0_REG, &reg);
    reg |= 0b10; // add clr_flt
    i2c_reg_write_byte_dt(&motor, CONFIG0_REG, reg);
}

bool Motor::didFault(uint8_t* fault_out) {
    uint8_t fault = 0;
    i2c_reg_read_byte_dt(&this->motor, 0, &fault);
    if (fault_out != NULL) {
        *fault_out = fault;
    }
    return (fault >> 7) == 1;
}

/**
 * Prints the values of all of the registers in the motor driver
 */
void Motor::regDump() {
    for (int i = 0; i <= 0x19; i++) {
        i2c_reg_read_byte_dt(&motor, i, &flt);
        printk("Reg%02x: %02x\n", i, flt);
    }
}

/**
 * Sets up the ripple counting on the motor driver by writing to the appropriate registers.
 */
void Motor::setupRippleCounting() {
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
void Motor::enableOVSProtection() {
    uint8_t regval;
    i2c_reg_read_byte_dt(&motor, CONFIG0_REG, &regval);
    regval |= 0b01100000;
    i2c_reg_write_byte_dt(&motor, CONFIG0_REG, regval);
}

/**
 * Disable overvoltage protection, and stall protection
 */
void Motor::disableOVSProtection() {
    uint8_t regval;
    i2c_reg_read_byte_dt(&motor, CONFIG0_REG, &regval);
    regval &= 0b10011111;
    i2c_reg_write_byte_dt(&motor, CONFIG0_REG, regval);
}

/**
 * Enable motor spinning, overvoltage protection, and stall protection
 */
void Motor::enableSpin() {
    uint8_t regval;
    i2c_reg_read_byte_dt(&motor, CONFIG0_REG, &regval);
    regval |= 0b10000000;
    i2c_reg_write_byte_dt(&motor, CONFIG0_REG, regval);
    enableOVSProtection();
}

/**
 * Disable motor spinning
 */
void Motor::disableSpin() {
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
void Motor::setSpinMode(Motor::SpinMode mode) {
    if (flip_voltage) {
        if (mode == SpinMode::Forward) {
            mode = SpinMode::Backward;
        } else if (mode == SpinMode::Backward) {
            mode = SpinMode::Forward;
        }
    }
    if (mode == SpinMode::Forward) {
        i2c_reg_write_byte_dt(&motor, CONFIG4_REG, 0x36);
    } else if (mode == SpinMode::Backward) {
        i2c_reg_write_byte_dt(&motor, CONFIG4_REG, 0x37);
    } else if (mode == SpinMode::Brake) {
        i2c_reg_write_byte_dt(&motor, CONFIG4_REG, 0x34);
    }
}

/**
 * Enables the soft start feature of the motor driver by writing to the appropriate register.
 */
void Motor::enableSoftStart() {
    uint8_t regval;
    i2c_reg_read_byte_dt(&motor, REG_CTRL0_REG, &regval);
    regval |= 0b00100000;
    i2c_reg_write_byte_dt(&motor, REG_CTRL0_REG, regval);
}

/**
 * Disables the soft start feature of the motor driver by writing to the appropriate register.
 */
void Motor::disableSoftStart() {
    uint8_t regval;
    i2c_reg_read_byte_dt(&motor, REG_CTRL0_REG, &regval);
    regval &= 0b11011111;
    i2c_reg_write_byte_dt(&motor, REG_CTRL0_REG, regval);
}

/**
 * Tests running the motor by controlling the voltage sent to it
 * @param dir the direction to run the motor in, 0 for forward and 1 for backward
 */
void Motor::voltageControlTest(uint8_t dir) {
    setToVoltageControlMode();
    setupRippleCounting();

    setSpinMode(SpinMode::Brake); // brake mode to start
    printFault();

    setSpeed(0);

    enableSpin();
    printFault();

    setSpinMode(SpinMode::Brake);
    printFault();

    for (int i = 4; i <= 12; i++) {
        setVoltage(i);
        printInfo();
        k_msleep(500);
    }

    setSpinMode(SpinMode::Brake); // brake mode to stop
}

/**
 * Tests running the motor by directly controlling the speed
 * @param dir the direction to run the motor in, 0 for forward and 1 for backward
 */
void Motor::speedControlTest(SpinMode dir) {
    setupRippleCounting();
    setToSpeedControlMode();

    setSpinMode(SpinMode::Brake); // brake mode to start
    printFault();

    setSpeed(0);

    enableSpin();
    printFault();

    setSpinMode(dir);
    printFault();

    for (int i = 1; i <= 10; i++) {
        setSpeed(i * 40);
        printInfo();
        k_msleep(2000);
    }

    setSpinMode(SpinMode::Brake); // brake mode to stop
}

/**
 * Pre Condition: i2c bus is ready and motor is connected to the bus
 * Sets up the motor driver to run in speed control mode
 */
int Motor::initSpeedControl() {
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
    setSpinMode(SpinMode::Brake); // default to brake mode
    setSpeed(0);
    return 1;
}

void Motor::setTInrush(uint16_t val) {
    i2c_reg_write_byte_dt(&motor, CONFIG1_REG, val & 0xff);
    i2c_reg_write_byte_dt(&motor, CONFIG1_REG, val >> 8);
}
/**
 * Sets up the motor driver to run in voltage control mode
 */
int Motor::initVoltageControl() {
    if (!device_is_ready(i2c_bus)) {
        printk("No i2c ready");
        return 0;
    }
    setTInrush(10);

    setToVoltageControlMode();
    setupRippleCounting();
    enableSoftStart();
    setSpinMode(SpinMode::Brake); // default to brake mode
    setVoltage(0);
    return 1;
}

/**
 * Reads the motor's quadrature encoder (if it has one)
 */
int64_t Motor::read_enc() {
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
    int64_t microdegrees = sensor_value_to_micro(&counter_val);
    if (flip_encoder){
        return (static_cast<int64_t>(360)*1000000)-microdegrees;
    }
    return microdegrees;
}
