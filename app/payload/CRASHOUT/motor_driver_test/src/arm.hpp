#pragma once
#include "common.hpp"

void arm_init();
void start_arm();

void start_arm_hold();
MovementResult step_arm_hold();
void stop_arm_hold();

MovementResult step_arm();
void stop_arm();
void set_arm_target(const ArmPose &);

void set_arm_pose(const ArmPose &);

void wakeup_motors();
void sleep_motors();