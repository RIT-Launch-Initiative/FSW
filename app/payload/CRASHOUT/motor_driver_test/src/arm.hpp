#pragma once
#include "common.hpp"

void arm_init();

// call > 10 times a second to accurately detect wraparounds
void arm_measure(const Vec3_16 &base_imu, const Vec3_32 &l2_imu);


void arm_set_ignore_stall(bool ignore);

void set_jog_paramters(const JogAction &);
void start_arm_jog();
MovementResult step_arm_jog();
void stop_arm_jog();


void start_arm_hold();
MovementResult step_arm_hold();
void stop_arm_hold();

void start_arm();
MovementResult step_arm();
void stop_arm();
void set_arm_target(const ArmPose &);

void set_arm_pose(const ArmPose &);



void wakeup_motors();
void sleep_motors();