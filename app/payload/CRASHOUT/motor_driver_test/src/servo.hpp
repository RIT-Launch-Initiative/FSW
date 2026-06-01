#pragma once
#include "common.hpp"
#include <zephyr/drivers/pwm.h>



struct Servo {
    struct pwm_dt_spec pwm;
    uint32_t closed_us;
    uint32_t open_us;
    uint32_t last_us;

    int disconnect();
    int open();
    int close();
    int set_us(uint32_t us);

};


void servo_init();
void set_servo_motion(FlipServo servoid, FlipServoMotion motion);

MovementResult step_servo(FlipServo servoid);
void stop_servo_move(FlipServo servoid);
void start_servo_move(FlipServo servoid);

void start_servo_hold();
void stop_servo_hold();
