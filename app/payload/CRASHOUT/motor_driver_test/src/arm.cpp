#include "arm.hpp"

#include "motor.hpp"
#include "servo.hpp"

#include <cmath>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(arm);

struct ShoulderEncoders;
struct ShoulderPosition {
    // in microdegrees
    int64_t yaw;
    int64_t pitch;

    ShoulderEncoders to_encoders() const;
    ShoulderPosition operator-(const ShoulderPosition &rhs) const { return {yaw - rhs.yaw, pitch - rhs.pitch}; }
    ShoulderPosition operator/(const int64_t &rhs) const { return {yaw / rhs, pitch / rhs}; }
    ShoulderPosition operator*(const int64_t &rhs) const { return {yaw * rhs, pitch * rhs}; }
};
struct ShoulderEncoders {
    // in microdegrees
    int64_t e1;
    int64_t e2;

    ShoulderPosition to_position() const {
        int64_t yaw = e1;
        int64_t pitch = -(e1 / 2) + e2 / 2;
        return {yaw, pitch};
    }
    ShoulderEncoders operator-(const ShoulderEncoders &rhs) const { return {e1 - rhs.e1, e2 - rhs.e2}; }
};

ShoulderEncoders ShoulderPosition::to_encoders() const {
    int64_t e1 = yaw;
    int64_t e2 = pitch * 2 + (e1 / 2);
    return {e1, e2};
}

// 90 degrees further than we expect bc time can't count as high as we want and overflow itnerrupts are annoying enought to add that i dont want to deal with it
constexpr int8_t LOCKIN_SHOULDER_YAW_ANGLE_B = 0;
constexpr int8_t LOCKIN_SHOULDER_PITCH_ANGLE_B = 90;
constexpr int8_t LOCKIN_ELBOW_ANGLE_B = -127;
constexpr int8_t LOCKIN_WRIST_ANGLE_B = -127;

static ArmPose current_pose = {LOCKIN_SHOULDER_YAW_ANGLE_B, LOCKIN_SHOULDER_PITCH_ANGLE_B, LOCKIN_ELBOW_ANGLE_B,
                               LOCKIN_WRIST_ANGLE_B};
static ShoulderEncoders current_shoulder_encoders{0, 0};
static ShoulderPosition current_shoulder_position{0, 0};

static ArmPose target_pose = {0, 0, 0, 0};
static ShoulderPosition target_shoulder{0, 0};
static ShoulderPosition initial_shoulder{0, 0};

JogAction jog_action{0};

uint32_t counter = 0;
ShoulderEncoders motor_zero_offset{0};

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

ShoulderEncoders shoulder_bytes_to_encoder_zero_point(const ShoulderEncoders &current_zeros, int64_t current_e1_ticks,
                                                      int64_t current_e2_ticks, int8_t yaw, int8_t pitch);

/**
 * Read shoulder encoders and compensate with zero offset
 */
ShoulderEncoders get_current_shoulder_encs() {
    int64_t e1 = motor1.read_enc() - motor_zero_offset.e1;
    int64_t e2 = motor2.read_enc() - motor_zero_offset.e2;
    return {e1, e2};
}

static constexpr int64_t DEG_TO_UDEG_MUL = 1000000;
int8_t shoulder_ang_microdegrees_to_byte(int64_t microdegrees) {
    int64_t ang = microdegrees / DEG_TO_UDEG_MUL;
    if (ang < -128) {
        return -128;
    } else if (ang > 127) {
        return 127;
    }
    return static_cast<int8_t>(ang);
}

void arm_measure() {
    current_shoulder_encoders = get_current_shoulder_encs();
    current_shoulder_position = current_shoulder_encoders.to_position();

    current_pose.shoulder_yaw = shoulder_ang_microdegrees_to_byte(current_shoulder_position.yaw);
    current_pose.shoulder_pitch = shoulder_ang_microdegrees_to_byte(current_shoulder_position.pitch);
}

void arm_init() {
    gpio_pin_configure_dt(&wrist_servo_enable, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&nSleep, GPIO_OUTPUT_INACTIVE);

    int64_t m1enc = motor1.read_enc();
    int64_t m2enc = motor2.read_enc();
    motor_zero_offset = shoulder_bytes_to_encoder_zero_point(
        motor_zero_offset, m1enc, m2enc, LOCKIN_SHOULDER_YAW_ANGLE_B, LOCKIN_SHOULDER_PITCH_ANGLE_B);
    arm_measure();
    // on activation,
    //  shoulder yaw = 0 deg
    //  shoulder pitch = 90 deg (90 b)
    // elbow pitch = -180 deg (-127 b)
    // wrist pitch = -90 deg (-127 b)
}

void wakeup_motors() { gpio_pin_set_dt(&nSleep, 1); }
void sleep_motors() { gpio_pin_set_dt(&nSleep, 0); }
void set_arm_target(const ArmPose &pose) {
    target_pose = pose;
    target_shoulder = ShoulderPosition{pose.shoulder_yaw * DEG_TO_UDEG_MUL, pose.shoulder_pitch * DEG_TO_UDEG_MUL};
}

ShoulderPosition pos_from_bytes(int8_t yaw, int8_t pitch) { return {yaw * DEG_TO_UDEG_MUL, pitch * DEG_TO_UDEG_MUL}; }

constexpr int64_t deg_to_udeg(int64_t val) { return val * DEG_TO_UDEG_MUL; }

void println_motordeg(int64_t m1, int64_t m2) {
    int64_t m1_deg = m1 / DEG_TO_UDEG_MUL;
    int64_t m1_frac = std::abs(m1) % DEG_TO_UDEG_MUL;
    int64_t m2_deg = m2 / DEG_TO_UDEG_MUL;
    int64_t m2_frac = std::abs(m2) % DEG_TO_UDEG_MUL;
    printk("%lld.%06lld, %lld.%06lld\n", m1_deg, m1_frac, m2_deg, m2_frac);
}

int64_t dot_sp(ShoulderPosition u, ShoulderPosition v) { return (u.yaw * v.yaw) + (u.pitch * v.pitch); }

ShoulderPosition proj1000(ShoulderPosition u, ShoulderPosition v) {
    // return np.dot(u, v) / np.linalg.norm(u)
    // num = (u/1)@(v/1)
    // denom = (u//1)@(u//1)
    int64_t num = dot_sp(u / 1000, v / 1000);   //u[0]*v[0] + u[1]*v[1]
    int64_t denom = dot_sp(u / 1000, u / 1000); // #u[0]*u[0] + u[1]*u[1]
    return ((u * num) / denom);
}

struct PathInfo {
    ShoulderPosition V;
    ShoulderPosition W;
    ShoulderPosition on;
    ShoulderPosition off;
};

PathInfo calcPathInfo(ShoulderPosition start, ShoulderPosition end, ShoulderPosition current) {
    ShoulderPosition V = end - start;
    // Vhat = V / np.linalg.norm(V)
    ShoulderPosition W = current - start;
    ShoulderPosition on = proj1000(V, W);
    ShoulderPosition off = W - on;
    return {V, W, on, off};
}
ShoulderEncoders shoulder_bytes_to_encoder_zero_point(const ShoulderEncoders &current_zeros, int64_t current_e1_ticks,
                                                      int64_t current_e2_ticks, int8_t yaw, int8_t pitch) {
    // if we were setting to 0,0, set e1_zero = current_e1_ticks, e2_zero = current_e2_ticks
    printk("Current zeros: ");
    println_motordeg(current_zeros.e1, current_zeros.e2);

    ShoulderEncoders current_motors{current_e1_ticks - current_zeros.e1, current_e2_ticks - current_zeros.e2};
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

    ShoulderEncoders delta_motors = delta_position.to_encoders();
    printk("Delta Motors: ");
    println_motordeg(delta_motors.e1, delta_motors.e2);

    int64_t e1_zerop = motor_zero_offset.e1 - delta_motors.e1;
    int64_t e2_zerop = motor_zero_offset.e2 - delta_motors.e2;

    return {e1_zerop, e2_zerop};
}

void set_arm_pose(const ArmPose &pose) {
    motor_zero_offset = shoulder_bytes_to_encoder_zero_point(motor_zero_offset, motor1.read_enc(), motor2.read_enc(),
                                                             pose.shoulder_yaw, pose.shoulder_pitch);
    current_pose = pose;
    printk("Set pose to %d %d %d %d\n", pose.shoulder_yaw, pose.shoulder_pitch, pose.elbow_pitch, pose.wrist_pitch);
    printk("Zeros at %lld %lld\n", motor_zero_offset.e1, motor_zero_offset.e2);
}

void set_wrist(int8_t angle) {
    int32_t range = (int32_t) wrist_servo.open_us - (int32_t) wrist_servo.closed_us;
    int32_t center = (wrist_servo.open_us + wrist_servo.closed_us) / 2;
    int32_t us = ((angle * range) / 256) + center;
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
        set_wrist(LOCKIN_WRIST_ANGLE_B);
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
    printk("Zeroed Motor at  ");
    println_motordeg(current_shoulder_encoders.e1, current_shoulder_encoders.e2);
    printk("Arm at           ");
    println_motordeg(current_shoulder_position.yaw, current_shoulder_position.pitch);
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
        initial_shoulder = current_shoulder_position;

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

    PathInfo path_info = calcPathInfo(initial_shoulder, target_shoulder, current_shoulder_position);
    int64_t start = k_uptime_ticks();
    printk("============\n");
    printk("Zeroed Motor at  ");
    println_motordeg(current_shoulder_encoders.e1, current_shoulder_encoders.e2);
    printk("Arm at           ");
    println_motordeg(current_shoulder_position.yaw, current_shoulder_position.pitch);

    printk("Targeting ");
    println_motordeg(target_shoulder.yaw, target_shoulder.pitch);

    printk("On amt ");
    println_motordeg(path_info.on.yaw, path_info.on.pitch);

    printk("Off amt ");
    println_motordeg(path_info.off.yaw, path_info.off.pitch);

    printk("Etar t           %d, %d, %d ,%d\n", target_pose.shoulder_yaw, target_pose.shoulder_pitch,
           target_pose.elbow_pitch, target_pose.wrist_pitch);
    printk("Earm t           %d  ,%d\n", current_pose.shoulder_yaw, current_pose.shoulder_pitch);
    int64_t el = k_uptime_ticks() - start;
    printk("el: %lld\n", k_ticks_to_us_near64(el));
    uint8_t this_fault = 0;

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
    printk("Stopping arm\n");
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