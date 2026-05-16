/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "arm.hpp"
#include "common.hpp"
#include "servo.hpp"

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER(main);

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* The devicetree node identifiers*/
#define LED1_NODE DT_NODELABEL(led1)

#define NSLEEP_NODE DT_NODELABEL(nsleep)
const struct device *const i2c_bus = DEVICE_DT_GET(DT_NODELABEL(motor_i2c));
const struct i2c_dt_spec motor1_i2c = {.bus = i2c_bus, .addr = 0x30};
const struct i2c_dt_spec motor2_i2c = {.bus = i2c_bus, .addr = 0x32};
const struct i2c_dt_spec motor3_i2c = {.bus = i2c_bus, .addr = 0x36};

const struct device *yaw_enc = DEVICE_DT_GET(DT_NODELABEL(yaw_enc));
const struct device *pitch_enc = DEVICE_DT_GET(DT_NODELABEL(pitch_enc));
const struct device *dcm_enc3 = DEVICE_DT_GET(DT_NODELABEL(dcm_enc3));

static const struct gpio_dt_spec nSleep = GPIO_DT_SPEC_GET(NSLEEP_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

#define MAIN_LOOP_TIME K_MSEC(10)
K_TIMER_DEFINE(main_loop_timer, NULL, NULL);

uint32_t base_accel_set_time = 0;
Vec3_16 base_accel = {0};

ArmPose arm_target = {0};

void handle_arm() { printk("doing arm things\n"); }

K_MSGQ_DEFINE(command_msgq, sizeof(InternalCommand), 4, alignof(InternalCommand));

int send_internal_command(InternalCommand *cmd) {
    return k_msgq_put(&command_msgq, static_cast<void *>(cmd), K_NO_WAIT);
}

static uint32_t iterations = 0;

int main() {
    int ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: failed to configure led pin \n", ret);
        return 0;
    }
    servo_init();
    printk("Setup LED\n");
    k_timer_start(&main_loop_timer, MAIN_LOOP_TIME, MAIN_LOOP_TIME);

    State state = State::Chilling;
    while (true) {
        iterations++;
        k_timer_status_sync(&main_loop_timer);
        StatusWord sw = MakeStatusWord(State::Chilling, false, false, false, false, false);
        SubmitStatus(sw, {0, 0, 0, 0});

        InternalCommand cmd;
        State next_state = state;
        if (k_msgq_get(&command_msgq, &cmd, K_NO_WAIT) == 0) {
            // handle message
            switch (cmd.kind) {
                case InternalCommandKind::Reset:
                    printk("REBOOT\n");
                    sys_reboot(SYS_REBOOT_COLD);
                    break;
                case InternalCommandKind::StartArm:
                    if (state == State::Chilling) {
                        next_state = State::ArmMoving;
                    } else {
                        LOG_WRN("Not starting arm bc not in resting state");
                    }
                    break;
                case InternalCommandKind::StartServo1:
                    if (state == State::Chilling) {
                        next_state = State::Servo1Moving;
                    } else {
                        LOG_WRN("Not starting servo 1 bc not in resting state");
                    }
                    break;
                case InternalCommandKind::StartServo2:
                    if (state == State::Chilling) {
                        next_state = State::Servo2Moving;
                    } else {
                        LOG_WRN("Not starting servo 2 bc not in resting state");
                    }
                    break;
                case InternalCommandKind::StartServo3:
                    if (state == State::Chilling) {
                        next_state = State::Servo3Moving;
                    } else {
                        LOG_WRN("Not starting servo 3 bc not in resting state");
                    }
                    break;
                case InternalCommandKind::Stop:
                    if (state == State::Chilling) {
                        LOG_WRN("Nothing to stop");
                    } else {
                        next_state = State::Chilling;
                    }
                    break;
                case InternalCommandKind::SetBaseAccel:
                    base_accel = cmd.set_base_accel;
                    base_accel_set_time = k_uptime_get();
                    break;
                case InternalCommandKind::SetArmTarget:
                    set_arm_target(cmd.set_arm_target);
                    break;
                case InternalCommandKind::SetServo1Motion:
                    set_servo_motion(FlipServo::Servo1, cmd.set_servo1_motion);
                    // printk("Set Servo Motion 1: %d %d %d %d %d\n", cmd.set_servo1_motion.open_duration, cmd.set_servo1_motion.openness, cmd.set_servo1_motion.open_travel_duration, cmd.set_servo1_motion.closedness, cmd.set_servo1_motion.close_travel_duration);
                    break;
                case InternalCommandKind::SetServo2Motion:
                    set_servo_motion(FlipServo::Servo2, cmd.set_servo2_motion);
                    break;
                case InternalCommandKind::SetServo3Motion:
                    set_servo_motion(FlipServo::Servo3, cmd.set_servo3_motion);
                    break;
                case InternalCommandKind::SetArmPose:
                    set_arm_pose(cmd.set_arm_pose);
            }
        }
        // otherwise, continue

        // things that happen no matter what
        // read temps
        // if motors on, read their positions
        //
        if (state != next_state) {
            switch (state) {
                case State::Chilling:
                    break;
                case State::ArmMoving:
                    stop_arm();
                    break;
                case State::Servo1Moving:
                    stop_servo_move(FlipServo::Servo1);
                    break;
                case State::Servo2Moving:
                    stop_servo_move(FlipServo::Servo2);
                    break;
                case State::Servo3Moving:
                    stop_servo_move(FlipServo::Servo3);
                    break;
            }
            switch (next_state) {
                case State::Chilling:
                    break;
                case State::ArmMoving:
                    start_arm();
                    break;
                case State::Servo1Moving:
                    start_servo_move(FlipServo::Servo1);
                    break;
                case State::Servo2Moving:
                    start_servo_move(FlipServo::Servo2);
                    break;
                case State::Servo3Moving:
                    start_servo_move(FlipServo::Servo3);
                    break;
            }
        }
        state = next_state;
        MovementResult move_result = MovementResult::Ongoing;
        switch (state) {
            case State::Chilling:
                // don't need to do anything
                break;
            case State::ArmMoving:
                step_arm();
                break;
            case State::Servo1Moving:
                move_result = step_servo(FlipServo::Servo1);
                break;
            case State::Servo2Moving:
                move_result = step_servo(FlipServo::Servo2);
                break;
            case State::Servo3Moving:
                move_result = step_servo(FlipServo::Servo3);
                break;
        }
        if (state != State::Chilling) {
            printk("state wasnt chilling\n");

            if (move_result != MovementResult::Ongoing) {
                printk("not ongoing anymore\n");
                switch (state) {
                    case State::Chilling:
                        // don't need to do anything
                        break;
                    case State::ArmMoving:
                        stop_arm();
                        break;
                    case State::Servo1Moving:
                        stop_servo_move(FlipServo::Servo1);
                        break;
                    case State::Servo2Moving:
                        stop_servo_move(FlipServo::Servo2);
                        break;
                    case State::Servo3Moving:
                        stop_servo_move(FlipServo::Servo3);
                        break;
                }
                state = State::Chilling;
            }
        }
    }
}

namespace CurrentState {
Temperatures temperatures() { return {.link1_temp = 1, .link2_temp = 2, .stm_temp = 3}; }
uint32_t current_iteration() { return iterations; }
} // namespace CurrentState
