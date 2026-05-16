#pragma once
#include "common.hpp"


void servo_init();
void set_servo_motion(FlipServo servoid, FlipServoMotion motion);

MovementResult step_servo(FlipServo servoid);
void stop_servo_move(FlipServo servoid);
void start_servo_move(FlipServo servoid);
