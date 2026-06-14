#pragma once
#include <cstdint>

enum class State {
    Chilling = 0,
    ArmMoving = 1,
    Servo1Moving = 2,
    Servo2Moving = 3,
    Servo3Moving = 4,
    Holding = 5,
    Jogging = 6,
};

using StatusWord = uint16_t;

enum StatusBit {
    StatusBitBooted = 0,  // set to 1 if board is ready
    StatusBit_State0 = 1, // Arm
    StatusBit_State1 = 2, // Flipping Servo 1
    StatusBit_State2 = 3, // Flipping Servo 2

    StatusBit_MovingArmFailed = 5,       // Arm failed bc OCP
    StatusBit_WristServoEn = 6,          // Efuse enable
    StatusBit_FlipServoEn = 7,           // 8.4V Buck enable
    StatusBit_MotorEn = 8,               // not sleeping
    StatusBitCantTrustImuLink = 9, // if we're on our side, the top joint doesnt know where it is
    StatusBitEncodersNotUpdating = 10,

    // identify what kind of response this is 0 - 31
    StatusBit_RType0 = 11,
    StatusBit_RType1 = 12,
    StatusBit_RType2 = 13,
    StatusBit_RType3 = 14,
    StatusBit_RType4 = 15,
};

struct ArmPose {
    int8_t shoulder_yaw;
    int8_t shoulder_pitch;
    int8_t elbow_pitch;
    int8_t wrist_pitch;
};

/**
 * 
 */
StatusWord MakeStatusWord(State state, bool wrist_en, bool flip_en, bool motor_en, bool movement_failed, bool overtemp);
void SubmitStatus(StatusWord word, const ArmPose &pose);

enum ResponseKind {
    ResponseKind_Status = 0,
    ResponseKind_Temps = 1,
    ResponseKind_Uptime = 2,
    ResponseKind_SOMETHING = 3,
    ResponseKind_BaseAccel = 4,
    ResponseKind_Link1Accel = 5,
    ResponseKind_Link2Accel = 6,

    ResponseKind_Servo1Motion = 7,
    ResponseKind_Servo2Motion = 8,
    ResponseKind_Servo3Motion = 9,

    ResponseKind_ArmPoseEst = 10,
    ResponseKind_ArmTarget = 11,

    ResponseKind_CurrentServoTargets = 12, // what we're saying since we don't have feedback

};

enum FlipServo {
    Servo1 = 0,
    Servo2 = 1,
    Servo3 = 2,
};

struct FlipServoMotion {
    uint8_t open_duration;         // How long to stay open for in 10 ms increments
    uint8_t openness;              // How much to open
    uint8_t open_travel_duration;  // How long to move the servo on open in 10 ms increments
    uint8_t closedness;            // where to go after open duration
    uint8_t close_travel_duration; // How long to move the servo on close in 10 ms increments

    uint32_t total_duration();
};

struct FlipServoMotionState {
    uint32_t iteration_started;
    FlipServoMotion motion;

    uint32_t pulse_length_at_iteration(uint32_t iteration, uint32_t min_pulse, uint32_t max_pulse);
};

struct Temperatures {
    int16_t link1_temp;
    int16_t link2_temp;
    int16_t stm_temp;
};

struct Vec3_16 {
    int16_t x;
    int16_t y;
    int16_t z;
};
struct Vec3_32 {
    int32_t x;
    int32_t y;
    int32_t z;
    Vec3_16 toMillig();
    Vec3_16 Saturated16();
};
int16_t saturate_i16(int32_t v);


bool getVerticalAngleFromImus(Vec3_16 base_imu16_normalized, Vec3_32 link_imu, int64_t yaw, int64_t pitch,
                              int64_t *microdeg_out);
Vec3_32 normalize_to16(Vec3_32 v);

struct ServoTargets {
    uint16_t servo1;
    uint16_t servo2;
    uint16_t servo3;
};

enum class MovementResult {
    Ongoing = 0,
    Finished = 1,
    Failed = 2,
    Invalid = 3,
};

struct JogAction {
    uint8_t motor;
    uint16_t iterations;
    int16_t millivolts;
};

/**
 * State that gets read by spi guy
 */

namespace CurrentState {
bool base_accel_is_valid();
uint32_t current_iteration();
bool wrist_en();
bool motors_en();
bool flip_en();
FlipServoMotion servo_motion(FlipServo servoid);

ArmPose arm_pose_est();
ArmPose arm_pose_target();

Temperatures temperatures();

uint32_t uptime();

Vec3_16 base_imu();
Vec3_16 link1_imu();
Vec3_16 link2_imu();

} // namespace CurrentState

enum class InternalCommandKind {
    Tick,
    Reset,
    StartHold,
    StartArm,
    StartArmNoCurrentLimit,
    StartServo1,
    StartServo2,
    StartServo3,
    StartJog,
    Stop,
    SetBaseAccel,
    SetArmTarget,
    SetServo1Motion,
    SetServo2Motion,
    SetServo3Motion,
    SetArmPose,
    SetJogMotion,
};

struct InternalCommand {
    InternalCommandKind kind;
    union {
        Vec3_16 set_base_accel;
        ArmPose set_arm_target;
        FlipServoMotion set_servo1_motion;
        FlipServoMotion set_servo2_motion;
        FlipServoMotion set_servo3_motion;
        ArmPose set_arm_pose;
        JogAction jog_action;
    };
};

int send_internal_command(InternalCommand *);