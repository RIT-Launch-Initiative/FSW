#pragma once
#include "common.hpp"


void start_arm();
void step_arm();
void stop_arm();
void set_arm_target(const ArmPose &);

void set_arm_pose(const ArmPose &);