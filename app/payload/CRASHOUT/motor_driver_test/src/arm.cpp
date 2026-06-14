#include "arm.hpp"

#include "motor.hpp"
#include "servo.hpp"

#include <cmath>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "f_core/utils/linear_fit.hpp"


LOG_MODULE_REGISTER(arm);

struct ShoulderEncoders;
struct ShoulderPosition {
    // in microdegrees
    int64_t yaw;
    int64_t pitch;

    ShoulderEncoders to_encoders() const;
    ShoulderPosition operator-() const { return {-yaw, -pitch}; }
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
        int64_t pitch = -(e1 / 2) + (e2 / 2);
        return {yaw, pitch};
    }
    ShoulderEncoders operator-(const ShoulderEncoders &rhs) const { return {e1 - rhs.e1, e2 - rhs.e2}; }

    int64_t to_motor1_udeg() {
        // e1 measures 60t yaw gear
        // 60(yaw) -> 15(m1 output) = x4
        // m1 output -> m1 input = x1000
        // overall 4000
        return e1 * 4 * 1000;
    }

    int64_t to_motor2_udeg() {
        // e2 measures (modified) 14 tooth bevel gear  which is equal to 30 tooth worm wheel
        // e2 -> worm input(m2 output) = x15
        // m2 output -> m1 input = 298
        // overall 4470
        return e2 * 15 * 298;
    }
};

// kP  =        (mV/udeg)
// ikP = 1/kP = (udeg/mV)
// udeg * ikP = mV
constexpr int16_t max_mv_shoulder = 12000; // mV
constexpr int16_t max_mv_elbow = 9000; // mV
int16_t clamp_voltage(int64_t mV, int64_t max_mv) {
    if (mV > max_mv) {
        return max_mv;
    } else if (mV < -max_mv) {
        return -max_mv;
    }
    return static_cast<int16_t>(mV);
}
constexpr int64_t ikP1 = 3000000; // udegm/mV
constexpr int64_t ikP2 = 3000000; // udegm/mV
constexpr int64_t ikP3 = 3000;    // udeg/mV

struct MotorVec {
    int64_t m1;
    int64_t m2;

    bool isOutOfBounds(const MotorVec &low_bounds, const MotorVec &high_bounds) {
        if (m1 > high_bounds.m1) {
            return true;
        } else if (m1 < low_bounds.m1) {
            return true;
        } else if (m2 > high_bounds.m2) {
            return true;
        } else if (m2 < low_bounds.m2) {
            return true;
        }
        return false;
    }
};

MotorVec clampMotVVec(const MotorVec &low_bounds, const MotorVec &high_bounds, const MotorVec &v) {
    // way too many floats
    // find extents that we'll be hitting
    int64_t ex = 0;
    int64_t ey = 0;
    if (v.m1 > 0) {
        ex = high_bounds.m1;
    } else {
        ex = low_bounds.m1;
    }

    if (v.m2 > 0) {
        ey = high_bounds.m2;
    } else {
        ey = low_bounds.m2;
    }

    if (v.m1 == 0) { //vertical line
        return MotorVec{0, ey};
    }
    if (v.m2 == 0) { //horizontal line
        return MotorVec{ex, 0};
    }
    // evil float version
    // tx = ex / v[0]
    // ty = ey / v[1]
    // hitVertFirst = tx <= ty

    // tx <= ty
    // ex / v[0] <= ey / v[1]
    // ex *v[1] <= ey *v[0]
    bool hitVertFirst = ex * v.m2 <= ey * v.m1;

    if (hitVertFirst) { // meet vertical edge first
        // return np.array([ex, (ex * v[1]) // v[0]])

        MotorVec intersect{ex, (ex * v.m2) / v.m1};
        return intersect;
    } else { //hit horizontal edge
        MotorVec intersect{(ey * v.m1) / v.m2, ey};
        return intersect;
    }
}

void safeBox(ShoulderEncoders offCorrection, ShoulderEncoders onCorrection, int16_t *m1_out, int16_t *m2_out) {
    // assuming voltage applied to m1 is proportional to speed of m1
    // is proportional in the same way that voltage aplied to m2 is to speed of m2
    // apply offCorrection first to bring back to inline
    // apply onCorrection as available to continue down the path

    // bound of box in motor space is just voltage
    // bound of box in encoder space is in udeg
    constexpr int64_t bound1e = max_mv_shoulder * ikP1; // mV * (udeg/mv) = udegm (udeg of motor)
    constexpr int64_t bound2e = max_mv_shoulder * ikP2; // mV * (udeg/mV) = udegm (udeg of motor)
    MotorVec low_bounds{-bound1e, -bound2e};
    MotorVec high_bounds{bound1e, bound2e};
    int64_t m1off = offCorrection.to_motor1_udeg();
    int64_t m2off = offCorrection.to_motor2_udeg();
    MotorVec motOff{m1off, m2off};
    // if (motOff.isOutOfBounds(low_bounds, high_bounds)) {
    // motOff = clampMotVVec(low_bounds, high_bounds, motOff);
    // }
    MotorVec onLowBounds{low_bounds.m1 - motOff.m1, low_bounds.m2 - motOff.m2};
    MotorVec onHighBounds{high_bounds.m1 - motOff.m1, high_bounds.m2 - motOff.m2};

    int64_t m1on = onCorrection.to_motor1_udeg();
    int64_t m2on = onCorrection.to_motor2_udeg();
    MotorVec motOn{m1on, m2on};
    // if (motOn.isOutOfBounds(onLowBounds, onHighBounds)) {
    // motOn = clampMotVVec(onLowBounds, onHighBounds, motOn);
    // }
    int64_t m1_udeg = motOff.m1 + motOn.m1;
    int64_t m2_udeg = motOff.m2 + motOn.m2;

    *m1_out = clamp_voltage(m1_udeg / ikP1, max_mv_shoulder);
    *m2_out = clamp_voltage(m2_udeg / ikP2, max_mv_shoulder);
}

ShoulderEncoders ShoulderPosition::to_encoders() const {
    int64_t e1 = yaw;
    int64_t e2 = pitch * 2 + (e1 / 2);
    return {e1, e2};
}

// 90 degrees further than we expect bc time can't count as high as we want and overflow itnerrupts are annoying enought to add that i dont want to deal with it
constexpr int8_t LOCKIN_SHOULDER_YAW_ANGLE_B = 0;
constexpr int8_t LOCKIN_SHOULDER_PITCH_ANGLE_B = 90;
constexpr int8_t LOCKIN_ELBOW_ANGLE_B = -127;
constexpr int8_t LOCKIN_WRIST_ANGLE_B = 127;

static ArmPose current_pose = {LOCKIN_SHOULDER_YAW_ANGLE_B, LOCKIN_SHOULDER_PITCH_ANGLE_B, LOCKIN_ELBOW_ANGLE_B,
                               LOCKIN_WRIST_ANGLE_B};
static ShoulderEncoders current_shoulder_encoders{0, 0};
static ShoulderPosition current_shoulder_position{0, 0};

static ArmPose target_pose = {0, 0, 0, 0};
static ShoulderPosition target_shoulder{0, 0};
static ShoulderPosition initial_shoulder{0, 0};
static uint32_t servo_travel_time = 0;
CMovingAverage<int64_t, 5> j2_angle_avg{LOCKIN_ELBOW_ANGLE_B};
static bool should_trust_j2 = false;
static bool ignoring_stall = false;
void arm_set_ignore_stall(bool ignore) { ignoring_stall = ignore; }

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

Servo wrist_servo{PWM_DT_SPEC_GET(DT_NODELABEL(servo4)), 2500, 500, 800};
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

int8_t elbow_ang_microdegrees_to_byte(int64_t microdegrees) {
    // elbow can actually go -180 to 180 so need to reserve space for that
    int64_t ang = microdegrees * 2 / 3 / DEG_TO_UDEG_MUL;
    if (ang < -128) {
        return -128;
    } else if (ang > 127) {
        return 127;
    }
    return static_cast<int8_t>(ang);
}

int64_t elbow_byte_to_ang_microdegrees(int8_t b) {
    // elbow can actually go -180 to 180 so need to reserve space for that
    int64_t ang = (int64_t) b * DEG_TO_UDEG_MUL * 3 / 2;
    constexpr int64_t u180 = 180 * DEG_TO_UDEG_MUL;
    if (ang < -u180) {
        return -u180;
    } else if (ang > u180) {
        return u180;
    }
    return static_cast<int64_t>(ang);
}

void arm_measure(const Vec3_16 &base_imu, const Vec3_32 &l2_imu) {
    current_shoulder_encoders = get_current_shoulder_encs();
    current_shoulder_position = current_shoulder_encoders.to_position();

    current_pose.shoulder_yaw = shoulder_ang_microdegrees_to_byte(current_shoulder_position.yaw);
    current_pose.shoulder_pitch = shoulder_ang_microdegrees_to_byte(current_shoulder_position.pitch);

    int64_t j2_udeg = 0;
    bool should_trust_base = CurrentState::base_accel_is_valid();
    bool should_trust_link = false;
    if (should_trust_base) {
        should_trust_link = getVerticalAngleFromImus(base_imu, l2_imu, current_shoulder_position.yaw,
                                                     current_shoulder_position.pitch, &j2_udeg);
    }
    should_trust_j2 = should_trust_link && should_trust_base;
    if (should_trust_j2) {
        j2_angle_avg.Feed(j2_udeg);
        current_pose.elbow_pitch = elbow_ang_microdegrees_to_byte(j2_angle_avg.Avg());
    } else {
        printk("Not trusting imu for second angle measuremnt Base %d, Link %d\n", should_trust_base, should_trust_link);
    }
}

void arm_init() {
    gpio_pin_configure_dt(&wrist_servo_enable, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&nSleep, GPIO_OUTPUT_INACTIVE);

    int64_t m1enc = motor1.read_enc();
    int64_t m2enc = motor2.read_enc();
    motor_zero_offset = shoulder_bytes_to_encoder_zero_point(
        motor_zero_offset, m1enc, m2enc, LOCKIN_SHOULDER_YAW_ANGLE_B, LOCKIN_SHOULDER_PITCH_ANGLE_B);
}

void wakeup_motors() { gpio_pin_set_dt(&nSleep, 1); }
void sleep_motors() { gpio_pin_set_dt(&nSleep, 0); }
void set_arm_target(const ArmPose &pose) {
    target_pose = pose;
    target_shoulder = ShoulderPosition{pose.shoulder_yaw * DEG_TO_UDEG_MUL, pose.shoulder_pitch * DEG_TO_UDEG_MUL};
}

ShoulderPosition pos_from_bytes(int8_t yaw, int8_t pitch) { return {yaw * DEG_TO_UDEG_MUL, pitch * DEG_TO_UDEG_MUL}; }

constexpr int64_t deg_to_udeg(int64_t val) { return val * DEG_TO_UDEG_MUL; }

void println_microdeg(int64_t m1, int64_t m2) {
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
    ShoulderPosition remaining;
    ShoulderPosition on;
    ShoulderPosition off;
};

PathInfo calcSholderPathInfo(ShoulderPosition start, ShoulderPosition end, ShoulderPosition current) {
    ShoulderPosition V = end - start;
    // Vhat = V / np.linalg.norm(V)
    ShoulderPosition W = current - start;
    ShoulderPosition on = proj1000(V, W);
    ShoulderPosition off = W - on;
    ShoulderPosition remaining = end - current;
    return {V, W, remaining, on, off};
}
ShoulderEncoders shoulder_bytes_to_encoder_zero_point(const ShoulderEncoders &current_zeros, int64_t current_e1_ticks,
                                                      int64_t current_e2_ticks, int8_t yaw, int8_t pitch) {
    // if we were setting to 0,0, set e1_zero = current_e1_ticks, e2_zero = current_e2_ticks
    printk("Current zeros: ");
    println_microdeg(current_zeros.e1, current_zeros.e2);

    ShoulderEncoders current_motors{current_e1_ticks - current_zeros.e1, current_e2_ticks - current_zeros.e2};
    printk("Current Zeroed Motors: ");
    println_microdeg(current_motors.e1, current_motors.e2);

    ShoulderPosition current_position = current_motors.to_position();
    printk("Current Position: ");
    println_microdeg(current_position.yaw, current_position.pitch);

    // what its saying our pose is
    ShoulderPosition setpt_position{yaw * DEG_TO_UDEG_MUL, pitch * DEG_TO_UDEG_MUL};
    printk("Setpt Position: ");
    println_microdeg(setpt_position.yaw, setpt_position.pitch);

    // from current pose to setpt pose
    ShoulderPosition delta_position = setpt_position - current_position;
    printk("Delta Position: ");
    println_microdeg(delta_position.yaw, delta_position.pitch);

    ShoulderEncoders delta_motors = delta_position.to_encoders();
    printk("Delta Motors: ");
    println_microdeg(delta_motors.e1, delta_motors.e2);

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

        motor1.enableSpin();
        motor2.enableSpin();
        motor3.enableSpin();

        motor1.setSpinMode(Motor::Brake);
        motor2.setSpinMode(Motor::Brake);
        motor3.setSpinMode(Motor::Brake);

        motor3.setStopOnStall(false);
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
    println_microdeg(current_shoulder_encoders.e1, current_shoulder_encoders.e2);
    printk("Arm at           ");
    println_microdeg(current_shoulder_position.yaw, current_shoulder_position.pitch);
    printk("Earm t           %d  ,%d\n", current_pose.shoulder_yaw, current_pose.shoulder_pitch);
    if (counter == MOTOR_STARTUP_TIME) {
        motors[jog_action.motor]->initVoltageControl();
        motors[jog_action.motor]->clearFault();
        motors[jog_action.motor]->enableSpin();
        printk("Starting motor at %d mv for %d iters", (int) jog_action.millivolts, jog_action.iterations);
        motors[jog_action.motor]->setDirAndVoltage16(jog_action.millivolts);
    } else if (static_cast<int>(counter) == jog_action.iterations - 20) {
        motors[jog_action.motor]->setVoltage(0.0);
    } else if (static_cast<int>(counter) == jog_action.iterations - 10) {
        motors[jog_action.motor]->setSpinMode(Motor::Brake);
    } else if (counter > jog_action.iterations) {
        return MovementResult::Finished;
    }
    uint8_t flt = 0;
    if (motor1.didFault(&flt)) {
        printk("MOTOR 1 FAULT: %02x", flt);
        motor1.printInfo();
    }
    if (motor2.didFault(&flt)) {
        printk("MOTOR 2 FAULT: %02x", flt);
        motor1.printInfo();
    }
    if (motor3.didFault(&flt)) {
        printk("MOTOR 3 FAULT: %02x", flt);
        motor1.printInfo();
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

        motor1.setTInrush(200); // ~20 ms
        motor2.setTInrush(200); // ~20 ms
        motor3.setTInrush(200); // ~20 ms

        motor1.regDump();
        set_wrist(target_pose.wrist_pitch);
        servo_travel_time = counter + std::abs(current_pose.wrist_pitch - target_pose.wrist_pitch); // TODO fill this
        motor1.clearFault();
        motor1.enableSpin();
        motor1.setSpinMode(Motor::Forward);

        motor2.clearFault();
        motor2.enableSpin();
        motor2.setSpinMode(Motor::Forward);

        motor3.clearFault();
        motor3.enableSpin();
        motor3.setSpinMode(Motor::Forward);
        motor3.setStopOnStall(false);
    }
    if (!should_trust_j2) {
        // failed bc we don't know where joint 2 is
        return MovementResult::Failed;
    }

    PathInfo path_info = calcSholderPathInfo(initial_shoulder, target_shoulder, current_shoulder_position);
    printk("============ %d\n", counter);
    // printk("Zeroed Motor at  ");
    // println_motordeg(current_shoulder_encoders.e1, current_shoulder_encoders.e2);
    printk("Arm at           ");
    println_microdeg(current_shoulder_position.yaw, current_shoulder_position.pitch);

    // printk("Targeting ");
    // println_motordeg(target_shoulder.yaw, target_shoulder.pitch);

    // printk("On amt ");
    // println_motordeg(path_info.on.yaw, path_info.on.pitch);

    printk("On togo amt ");
    ShoulderPosition onTogo = path_info.V - path_info.on;
    println_microdeg(onTogo.yaw, onTogo.pitch);

    printk("Off amt ");
    println_microdeg(-path_info.off.yaw, -path_info.off.pitch);

    ShoulderEncoders offTogoE = (-path_info.off).to_encoders();
    printk("Needed off Change in Encoder space ");
    println_microdeg(offTogoE.e1, offTogoE.e2);

    ShoulderEncoders onTogoE = onTogo.to_encoders();
    // printk("Needed on Change in Encoder space ");
    // println_motordeg(onTogoE.e1, onTogoE.e2);

    printk("Elbow: going (%db) ", (int) target_pose.elbow_pitch);
    int64_t j2_angle = j2_angle_avg.Avg();
    println_microdeg(j2_angle, elbow_byte_to_ang_microdegrees(target_pose.elbow_pitch));
    int64_t elbow_error = elbow_byte_to_ang_microdegrees(target_pose.elbow_pitch) - j2_angle;

    static constexpr int64_t elbow_deadband = 3 * DEG_TO_UDEG_MUL;
    static constexpr int64_t deadband = 2 * DEG_TO_UDEG_MUL;
    bool on_target = (std::abs(path_info.remaining.yaw) < deadband) &&
                     (std::abs(path_info.remaining.pitch) < deadband) && (std::abs(elbow_error) < elbow_deadband) &&
                     (counter > servo_travel_time);
    if (on_target) {
        printk("Stopping bc finished :)\n");
        current_pose.wrist_pitch = target_pose.wrist_pitch;
        return MovementResult::Finished;
    }
    if (counter > 1500) {
        printk("Stopping bc internal timeout\n");
        current_pose.wrist_pitch = target_pose.wrist_pitch;
        return MovementResult::Failed;
    }

    printk("Earm t           %d, %d, %d, %d\n", current_pose.shoulder_yaw, current_pose.shoulder_pitch,
           current_pose.elbow_pitch, current_pose.wrist_pitch);
    printk("Etar t           %d, %d, %d ,%d\n", target_pose.shoulder_yaw, target_pose.shoulder_pitch,
           target_pose.elbow_pitch, target_pose.wrist_pitch);
    uint8_t this_fault = 0;

    int16_t m1out = 0;
    int16_t m2out = 0;
    int16_t m3out = clamp_voltage(elbow_error / ikP3, max_mv_elbow);

    safeBox(offTogoE, onTogoE, &m1out, &m2out);

    printk("On Correction (motor) ");
    println_microdeg(onTogoE.to_motor1_udeg(), onTogoE.to_motor2_udeg());
    printk("Off Correction (motor) ");
    println_microdeg(offTogoE.to_motor1_udeg(), offTogoE.to_motor2_udeg());
    printk("Elbow Error %lld\n", elbow_error);
    printk("Set Motor volts: %d %d %d\n", m1out, m2out, m3out);

    motor1.setDirAndVoltage16(m1out);
    motor2.setDirAndVoltage16(m2out);
    motor3.setDirAndVoltage16(m3out);

    if (motor1.didFault(&this_fault)) {
        printk("MOTOR 1 FAULT: %02x ", this_fault);
        motor1.printInfo();
        if (!ignoring_stall) {
            return MovementResult::Failed;
        }    }
    if (motor2.didFault(&this_fault)) {
        printk("MOTOR 2 FAULT: %02x ", this_fault);
        motor2.printInfo();
        if (!ignoring_stall) {
            return MovementResult::Failed;
        }    }
    if (motor3.didFault(&this_fault)) {
        printk("MOTOR 3 FAULT: %02x ", this_fault);
        motor3.printInfo();
        // dont fail on motor 3 stall
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