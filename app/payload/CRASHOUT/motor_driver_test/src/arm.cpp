#include "arm.hpp"

#include "motor.hpp"
#include "servo.hpp"

#include <cmath>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(arm);

struct ShoulderMotors;
struct ShoulderPosition {
    // in microdegrees
    int64_t yaw;
    int64_t pitch;

    ShoulderMotors to_motors() const;
    ShoulderPosition operator-(const ShoulderPosition &rhs) const { return {yaw - rhs.yaw, pitch - rhs.pitch}; }
};
struct ShoulderMotors {
    // in microdegrees
    int64_t e1;
    int64_t e2;

    ShoulderPosition to_position() const {
        int64_t yaw = e1;
        int64_t pitch = -(e1 / 2) + e2 / 2;
        return {yaw, pitch};
    }
    ShoulderMotors operator-(const ShoulderMotors &rhs) const { return {e1 - rhs.e1, e2 - rhs.e2}; }
};

ShoulderMotors ShoulderPosition::to_motors() const {
    int64_t e1 = yaw;
    int64_t e2 = pitch * 2 + (e1 / 2);
    return {e1, e2};
}

static ArmPose current_pose = {0, 0, 0, 0};
static ArmPose target_pose = {0, 0, 0, 0};
static ShoulderPosition target_shoulder{0, 0};

JogAction jog_action{0};

uint32_t counter = 0;
ShoulderMotors motor_zero_offset{0};

#define NSLEEP_NODE DT_NODELABEL(nsleep)
static const struct gpio_dt_spec nSleep = GPIO_DT_SPEC_GET(NSLEEP_NODE, gpios);

const struct device *yaw_enc = DEVICE_DT_GET(DT_NODELABEL(yaw_enc));
const struct device *pitch_enc = DEVICE_DT_GET(DT_NODELABEL(pitch_enc));

const struct device *const i2c_bus = DEVICE_DT_GET(DT_NODELABEL(motor_i2c));
Motor motor1(i2c_bus, 0x30, false, yaw_enc, false);
Motor motor2(i2c_bus, 0x32, true, pitch_enc, true);
Motor motor3(i2c_bus, 0x36, true);

Motor *motors[] = {&motor1, &motor2, &motor3};

Servo wrist_servo{PWM_DT_SPEC_GET(DT_NODELABEL(servo4)), 500, 2500, 800};
const gpio_dt_spec wrist_servo_enable = GPIO_DT_SPEC_GET(DT_NODELABEL(wrist_servo_pwr_en), gpios);

void shoulder_bytes_to_encoder_zero_point(int64_t current_e1_zero, int64_t current_e1_ticks, int64_t current_e2_zero,
                                          int64_t current_e2_ticks, int8_t yaw, int8_t pitch, int64_t *e1_zero,
                                          int64_t *e2_zero);

void arm_init() {
    gpio_pin_configure_dt(&wrist_servo_enable, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&nSleep, GPIO_OUTPUT_INACTIVE);

    int64_t m1enc = motor1.read_enc();
    int64_t m2enc = motor2.read_enc();
    shoulder_bytes_to_encoder_zero_point(motor_zero_offset.e1, m1enc, motor_zero_offset.e2, m2enc, 0, 90,
                                         &motor_zero_offset.e1, &motor_zero_offset.e2)

    // on activation,
    //  shoulder yaw = 0 deg
    //  shoulder pitch = 90 deg (90 b)
    // elbow pitch = -180 deg (-127 b)
    // wrist pitch = -90 deg (-127 b)
}

void wakeup_motors() { gpio_pin_set_dt(&nSleep, 1); }
void sleep_motors() { gpio_pin_set_dt(&nSleep, 0); }
void set_arm_target(const ArmPose &pose) { target_pose = pose; }

static constexpr int64_t DEG_TO_UDEG_MUL = 1000000;
ShoulderPosition pos_from_bytes(int8_t yaw, int8_t pitch) { return {yaw * DEG_TO_UDEG_MUL, pitch * DEG_TO_UDEG_MUL}; }

constexpr int64_t deg_to_udeg(int64_t val) { return val * DEG_TO_UDEG_MUL; }

int8_t shoulder_ang_microdegrees_to_byte(int64_t microdegrees) {
    int64_t ang = microdegrees / DEG_TO_UDEG_MUL;
    if (ang < -128) {
        return -128;
    } else if (ang > 127) {
        return 127;
    }
    return static_cast<int8_t>(ang);
}

int64_t udeg_to_360_to180(int64_t to_360) {
    constexpr int64_t u180 = 180 * DEG_TO_UDEG_MUL;
    to_360 %= (360 * DEG_TO_UDEG_MUL);
    if (to_360 > u180) {
        return -u180 + (to_360 - u180);
    }
    return to_360;
}

ShoulderMotors get_current_motor_encs() {
    printk("M1: %lld M2: %lld\n", motor1.read_enc(), motor2.read_enc());
    int64_t e1 = udeg_to_360_to180(motor1.read_enc() - motor_zero_offset.e1);
    int64_t e2 = udeg_to_360_to180(motor2.read_enc() - motor_zero_offset.e2);
    return {e1, e2};
}

void println_motordeg(int64_t m1, int64_t m2) {
    int64_t m1_deg = m1 / DEG_TO_UDEG_MUL;
    int64_t m1_frac = std::abs(m1) % DEG_TO_UDEG_MUL;
    int64_t m2_deg = m2 / DEG_TO_UDEG_MUL;
    int64_t m2_frac = std::abs(m2) % DEG_TO_UDEG_MUL;
    printk("%lld.%06lld, %lld.%06lld\n", m1_deg, m1_frac, m2_deg, m2_frac);
}
void shoulder_bytes_to_encoder_zero_point(int64_t current_e1_zero, int64_t current_e1_ticks, int64_t current_e2_zero,
                                          int64_t current_e2_ticks, int8_t yaw, int8_t pitch, int64_t *e1_zero,
                                          int64_t *e2_zero) {
    // if we were setting to 0,0, set e1_zero = current_e1_ticks, e2_zero = current_e2_ticks
    printk("Current zeros: ");
    println_motordeg(motor_zero_offset.e1, motor_zero_offset.e2);

    ShoulderMotors current_motors{udeg_to_360_to180(current_e1_ticks - motor_zero_offset.e1),
                                  udeg_to_360_to180(current_e2_ticks - motor_zero_offset.e2)};
    printk("Current Zeroed Motors: ");
    println_motordeg(current_motors.e1, current_motors.e2);

    ShoulderPosition current_position = current_motors.to_position();
    printk("Current Position: ");
    println_motordeg(current_position.yaw, current_position.pitch);

    // what its saying our pose is
    ShoulderPosition setpt_position{yaw * DEG_TO_UDEG_MUL, pitch * DEG_TO_UDEG_MUL};
    printk("Setpt Position: ");
    println_motordeg(setpt_position.yaw, setpt_position.pitch);

    // from current pose to setpt pose
    ShoulderPosition delta_position = setpt_position - current_position;
    printk("Delta Position: ");
    println_motordeg(delta_position.yaw, delta_position.pitch);

    ShoulderMotors delta_motors = delta_position.to_motors();
    printk("Delta Motors: ");
    println_motordeg(delta_motors.e1, delta_motors.e2);

    int64_t e1_zerop = motor_zero_offset.e1 - delta_motors.e1;
    int64_t e2_zerop = motor_zero_offset.e2 - delta_motors.e2;

    *e1_zero = e1_zerop;
    *e2_zero = e2_zerop;
}

void set_arm_pose(const ArmPose &pose) {
    shoulder_bytes_to_encoder_zero_point(motor_zero_offset.e1, motor1.read_enc(), motor_zero_offset.e2,
                                         motor2.read_enc(), pose.shoulder_yaw, pose.shoulder_pitch,
                                         &motor_zero_offset.e1, &motor_zero_offset.e2);
    current_pose = pose;
    printk("Set pose to %d %d %d %d\n", pose.shoulder_yaw, pose.shoulder_pitch, pose.elbow_pitch, pose.wrist_pitch);
    printk("Zeros at %lld %lld\n", motor_zero_offset.e1, motor_zero_offset.e2);
}

void set_wrist(int8_t angle) {
    int32_t range = wrist_servo.open_us - wrist_servo.closed_us;
    int32_t center = (wrist_servo.open_us + wrist_servo.closed_us) / 2;
    int32_t us = ((angle * range) / 256) + center;
    printk("Wrist Servo Set to %d", us);
    wrist_servo.set_us(us);
}

constexpr size_t MOTOR_STARTUP_TIME = 5;
void start_arm() {
    printk("Starting arm\n");
    gpio_pin_set_dt(&wrist_servo_enable, 1);
    wakeup_motors();
    counter = 0;
}

void start_arm_hold() {
    // just turn the stuff on
    start_arm();
}
MovementResult step_arm_hold() {
    counter++;
    if (counter == MOTOR_STARTUP_TIME) {
        motor1.initVoltageControl();
        motor2.initVoltageControl();
        motor3.initVoltageControl();
        // set_wrist(); // TODO what is hold location for wrist
        motor1.setSpinMode(Motor::Brake);
        motor2.setSpinMode(Motor::Brake);
        motor3.setSpinMode(Motor::Brake);
    }
    return MovementResult::Ongoing;
}
void stop_arm_hold() {
    wrist_servo.disconnect();
    gpio_pin_set_dt(&wrist_servo_enable, 0);

    sleep_motors();
}

void set_jog_paramters(const JogAction &params) {
    if (params.motor > 2) {
        LOG_WRN("Invalid motor for jog params: %d", params.motor);
        return;
    }
    printk("Received jog req: motor %d %d mV for %d iters", jog_action.motor, jog_action.millivolts,
           jog_action.iterations);
    jog_action = params;
}
void start_arm_jog() {
    // just turn the stuff on
    printk("Starting jog for %d iters at %d mv", jog_action.iterations, jog_action.millivolts);
    start_arm();
}

MovementResult step_arm_jog() {
    counter++;
    ShoulderMotors motorsP = get_current_motor_encs();
    ShoulderPosition pos = motorsP.to_position();
    current_pose.shoulder_yaw = shoulder_ang_microdegrees_to_byte(pos.yaw);
    current_pose.shoulder_pitch = shoulder_ang_microdegrees_to_byte(pos.pitch);
    printk("Zeroed Motor at  ");
    println_motordeg(motorsP.e1, motorsP.e2);
    printk("Arm at           ");
    println_motordeg(pos.yaw, pos.pitch);
    printk("Earm t           %d  ,%d\n", current_pose.shoulder_yaw, current_pose.shoulder_pitch);

    if (counter == MOTOR_STARTUP_TIME) {
        motors[jog_action.motor]->initVoltageControl();
        motors[jog_action.motor]->clearFault();
        motors[jog_action.motor]->enableSpin();
        motors[jog_action.motor]->setSpinMode(jog_action.millivolts < 0 ? Motor::Backward : Motor::Forward);
        uint16_t mv = std::abs(jog_action.millivolts);
        printk("Starting motor at %d mv for %d iters", (int) mv, jog_action.iterations);
        motors[jog_action.motor]->setVoltage16(mv);

    } else if (counter == jog_action.iterations - 20) {
        motors[jog_action.motor]->setVoltage(0.0);
        motors[jog_action.motor]->setSpinMode(Motor::Brake);
    } else if (counter > jog_action.iterations) {
        return MovementResult::Finished;
    }
    return MovementResult::Ongoing;
}
void stop_arm_jog() {
    motors[jog_action.motor]->setVoltage(0.0);
    motors[jog_action.motor]->setSpinMode(Motor::Brake);
    motors[jog_action.motor]->disableSpin();
    sleep_motors();
}

MovementResult step_arm() {
    counter++;
    if (counter < MOTOR_STARTUP_TIME) {
        printk("waiting to start: %lld\n", k_uptime_get());
        return MovementResult::Ongoing;
    } else if (counter == MOTOR_STARTUP_TIME) {
        motor1.initVoltageControl();
        motor2.initVoltageControl();
        motor3.initVoltageControl();
        motor1.regDump();
        set_wrist(target_pose.wrist_pitch);

        motor1.clearFault();
        motor1.printFault();
        motor1.enableSpin();
        motor1.setSpinMode(Motor::Forward);

        motor2.clearFault();
        motor2.enableSpin();
        motor2.setSpinMode(Motor::Forward);

        motor3.clearFault();
        motor3.enableSpin();
        motor3.setSpinMode(Motor::Brake);
    }

    ShoulderMotors motorsP = get_current_motor_encs();
    ShoulderPosition pos = motorsP.to_position();
    current_pose.shoulder_yaw = shoulder_ang_microdegrees_to_byte(pos.yaw);
    current_pose.shoulder_pitch = shoulder_ang_microdegrees_to_byte(pos.pitch);
    printk("Zeroed Motor at  ");
    println_motordeg(motorsP.e1, motorsP.e2);
    printk("Arm at           ");
    println_motordeg(pos.yaw, pos.pitch);
    printk("Earm t           %d  ,%d\n", current_pose.shoulder_yaw, current_pose.shoulder_pitch);

    motor1.printFault();
    uint8_t this_fault = 0;
    motor2.printInfo();
    if (motor1.didFault(&this_fault)) {
        printk("MOTOR 1 FAULT: %02x", this_fault);
        // return MovementResult::Failed;
    }
    if (motor2.didFault(&this_fault)) {
        printk("MOTOR 2 FAULT: %02x", this_fault);
        // return MovementResult::Failed;
    }
    if (motor3.didFault(&this_fault)) {
        printk("MOTOR 3 FAULT: %02x", this_fault);
        // return MovementResult::Failed;
    }
    if (counter > 100) {
        current_pose.wrist_pitch = target_pose.wrist_pitch;
        return MovementResult::Finished;
    }
    return MovementResult::Ongoing;
}

void stop_arm() {
    printk("Stopping arm");
    wrist_servo.disconnect();
    gpio_pin_set_dt(&wrist_servo_enable, 0);

    motor1.setVoltage(0.0);
    motor1.setSpinMode(Motor::Brake);
    motor1.disableSpin();
    motor2.setVoltage(0.0);
    motor2.setSpinMode(Motor::Brake);
    motor2.disableSpin();

    motor3.setVoltage(0.0);
    motor3.setSpinMode(Motor::Brake);
    motor3.disableSpin();

    sleep_motors();
}

namespace CurrentState {

bool wrist_en() { return gpio_pin_get_dt(&wrist_servo_enable); }
bool motors_en() { return gpio_pin_get_dt(&nSleep); }

ArmPose arm_pose_est() { return current_pose; }
ArmPose arm_pose_target() { return target_pose; }

}; // namespace CurrentState