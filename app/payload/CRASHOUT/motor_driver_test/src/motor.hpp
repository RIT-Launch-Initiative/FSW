/*
 * @file motor.cpp
 * 
 * Motor Class for controlling the motor driver of a single motor
 * Contains methods for enabling and disabling various settings of the motor driver
 * Can control the motor in both voltage control mode and speed control mode
 *
 * @author Collin Casey, Richard Sommers
 */
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

class Motor {
  public:
    /* Global variables */
    const struct device* i2c_bus;
    struct i2c_dt_spec motor;
    const struct device* enc;
    bool flip_voltage = false;
    bool flip_encoder = false;
    int32_t full_enc_rotations = 0;
    int64_t last_microdegrees = 0;
    uint8_t flt = 0;

    // w_scale_value is used to convert the speed value read from the motor driver in Rad/s to RPM
    // The default value 16 is the minimum value and maxes the motor out at 4080 Rad/s (Page 31 of datatsheet)
    static constexpr uint8_t DEFAULT_W_SCALE_VALUE = 16;
    uint8_t w_scale_value = DEFAULT_W_SCALE_VALUE;

    /**
     * Contructs an instance of the Motor class with the given i2c address.
     */
    Motor(const struct device* i2c_bus, uint8_t addr, bool flip_voltage);

    /**
     * Contructs an instance of the Motor class with the given i2c device specification.
     */
    Motor(const struct device* i2c_bus, struct i2c_dt_spec motor_spec, bool flip_voltage);


    /**
     * Contructs an instance of the Motor class with the given i2c address and encoder.
     */
    Motor(const struct device* i2c_bus, uint8_t addr, bool flip_voltage, const struct device* dcm_enc, bool flip_encoder);

    /**
     * Contructs an instance of the Motor class with the given i2c device specification and encoder.
     */
    Motor(const struct device* i2c_bus, struct i2c_dt_spec motor_spec, bool flip_voltage, const struct device* dcm_enc, bool flip_encoder);

    /**
     * Sets the voltage of the motor driver by writing to the appropriate register.
     */
    void setVoltage(float volts);
    void setVoltage16(uint16_t millivolts);
    void setDirAndVoltage16(int16_t millivolts);

    void setTInrush(uint16_t val);
    /**
     * Sets the w_scale value of the motor driver by writing to the appropriate register 
     * if w_scale given is valid (16, 32, 64, or 128).
     */
    void set_w_scale_value(uint32_t w_scale);

    /**
     * Sets the speed of the motor driver by writing to the appropriate register.
     * @param radps the speed to set the motor to in Rad/s
     */
    void setSpeed(uint16_t radps);

    /**
     * Converts the voltage value read from the motor driver to volts.
     * The value 228 is the maximum value that can be read from the motor driver and corresponds to 38 volts (Page 45 of datasheet).
     */
    float map_voltage(uint8_t vmtr);

    /**
     * Converts the speed value read from the motor driver in Rad/s to RPM.
     */
    float speed_rpm(uint8_t speed, int32_t w_scale);

    /**
     * Sets the motor driver to speed control mode
     */
    void setToSpeedControlMode();

    /**
     * Sets the motor driver to voltage control mode
     */
    void setToVoltageControlMode();

    void setStopOnStall(bool enabled);

    /**
     * Reads and prints the voltage, speed, ripple count, and direction from the motor driver.
     */
    void printInfo();

    /**
     * Reads and prints the fault register from the motor driver.
     */
    void printFault();
    void clearFault();
    bool didFault(uint8_t *fault);


    /**
     * Prints the values of all of the registers in the motor driver
     */
    void regDump();

    /**
     * Sets up the ripple counting on the motor driver by writing to the appropriate registers.
     */
    void setupRippleCounting();

    /**
     * Enable overvoltage protection, and stall protection
     */
    void enableOVSProtection();

    /**
     * Disable overvoltage protection, and stall protection
     */
    void disableOVSProtection();

    /**
     * Enable motor spinning, overvoltage protection, and stall protection
     */
    void enableSpin();
    /**
     * Disable motor spinning
     */
    void disableSpin();



    enum SpinMode {
        Forward,
        Backward, 
        Brake,
    };
    /**
     * Sets the spin mode of the motor driver by writing to the appropriate register.
     * @param mode: 0 = forward, 1 = backward, 2 = brake
     * We don't actually know which way forward and backward are, because it depends on how the motor is wired
     */
    void setSpinMode(SpinMode mode);

    /**
     * Enables the soft start feature of the motor driver by writing to the appropriate register.
     */
    void enableSoftStart();

    /**
     * Disables the soft start feature of the motor driver by writing to the appropriate register.
     */
    void disableSoftStart();

    /**
     * Tests running the motor by controlling the voltage sent to it
     * @param dir the direction to run the motor in, 0 for forward and 1 for backward
     */
    void voltageControlTest(uint8_t dir);

    /**
     * Tests running the motor by directly controlling the speed
     * @param dir the direction to run the motor in
     */
    void speedControlTest(SpinMode dir);

    /**
     * Pre Condition: i2c bus is ready and motor is connected to the bus
     * Sets up the motor driver to run in speed control mode
     */
    int initSpeedControl();

    /**
     * Sets up the motor driver to run in voltage control mode
     */
    int initVoltageControl();

    /**
     * Reads the motor's quadrature encoder to microdegrees (if it has one)
     */
    int64_t read_enc();
};