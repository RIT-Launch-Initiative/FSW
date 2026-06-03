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
const device *stm_temp = DEVICE_DT_GET(DT_NODELABEL(die_temp));
const device *l2_imu = DEVICE_DT_GET(DT_NODELABEL(link2_imu));
const device *imu_bus = DEVICE_DT_GET(DT_NODELABEL(imu_i2c));

static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

uint32_t base_accel_set_time = 0;
Vec3_16 base_accel = {0};

Vec3_32 l2_accel{0};

void imu_read_to_v32_normalize(sensor_value *xyz, Vec3_32 &vec) {
    vec.x = sensor_value_to_micro(&xyz[0]);
    vec.y = sensor_value_to_micro(&xyz[1]);
    vec.z = sensor_value_to_micro(&xyz[2]);
}

int8_t stored_stm_temp = 0;

K_MSGQ_DEFINE(command_msgq, sizeof(InternalCommand), 4, alignof(InternalCommand));

int send_internal_command(InternalCommand *cmd) {
    return k_msgq_put(&command_msgq, static_cast<void *>(cmd), K_NO_WAIT);
}

void tick_timer_handler(struct k_timer *dummy) {
    InternalCommand cmd{};
    cmd.kind = InternalCommandKind::Tick;
    send_internal_command(&cmd);
}

#define MAIN_LOOP_TIME K_MSEC(10)
K_TIMER_DEFINE(main_loop_timer, tick_timer_handler, NULL);

static uint32_t iterations = 0;

int main() {
    int ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: failed to configure led pin \n", ret);
        return 0;
    }
    i2c_recover_bus(imu_bus);
    arm_init();
    servo_init();
    printk("Setup LED\n");
    k_timer_start(&main_loop_timer, MAIN_LOOP_TIME, MAIN_LOOP_TIME);

    State state = State::Chilling;
    while (true) {
        StatusWord sw = MakeStatusWord(state, CurrentState::wrist_en(), CurrentState::flip_en(),
                                       CurrentState::motors_en(), false, false);

        gpio_pin_toggle_dt(&led1);

        // printk("%lld ms\n", k_uptime_get());
        // printk("SW: %04x  %d\n", sw, state);
        SubmitStatus(sw, CurrentState::arm_pose_est());

        InternalCommand cmd;
        State next_state = state;
        if (k_msgq_get(&command_msgq, &cmd, K_FOREVER) == 0) {
            // handle message
            switch (cmd.kind) {
                case InternalCommandKind::Tick:
                    iterations++;
                    break;
                case InternalCommandKind::Reset:
                    printk("REBOOT\n");
                    sys_reboot(SYS_REBOOT_COLD);
                    break;
                case InternalCommandKind::StartJog:
                    if (state == State::Chilling) {
                        printk("Start Jog main");
                        next_state = State::Jogging;
                    } else {
                        LOG_WRN("Not starting jog bc not in resting state");
                    }
                    break;
                case InternalCommandKind::StartHold:
                    if (state == State::Chilling) {
                        next_state = State::Holding;
                    } else {
                        LOG_WRN("Not starting hold bc not in resting state");
                    }
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
                        printk("Not starting servo 2 bc not in resting state");
                    }
                    break;
                case InternalCommandKind::StartServo3:
                    if (state == State::Chilling) {
                        next_state = State::Servo3Moving;
                    } else {
                        printk("Not starting servo 3 bc not in resting state");
                    }
                    break;
                case InternalCommandKind::Stop:
                    if (state == State::Chilling) {
                        printk("Nothing to stop");
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
                    break;
                case InternalCommandKind::SetServo2Motion:
                    set_servo_motion(FlipServo::Servo2, cmd.set_servo2_motion);
                    break;
                case InternalCommandKind::SetServo3Motion:
                    set_servo_motion(FlipServo::Servo3, cmd.set_servo3_motion);
                    break;
                case InternalCommandKind::SetArmPose:
                    set_arm_pose(cmd.set_arm_pose);
                    break;
                case InternalCommandKind::SetJogMotion:
                    LOG_INF("Setting jog motion");
                    set_jog_paramters(cmd.jog_action);
                    break;
            }
        }

        // things that happen no matter what
        arm_measure();
        // read temps
        sensor_sample_fetch(stm_temp);
        sensor_value die_temp{};
        sensor_channel_get(stm_temp, SENSOR_CHAN_DIE_TEMP, &die_temp);
        stored_stm_temp = static_cast<int8_t>(die_temp.val1);

        int iret = sensor_sample_fetch(l2_imu);
        if (iret == 0) {
            sensor_value l2_reading[3] = {0};
            int gret = sensor_channel_get(l2_imu, SENSOR_CHAN_ACCEL_XYZ, &l2_reading[0]);
            if (gret == 0) {
                imu_read_to_v32_normalize(l2_reading, l2_accel);
            } else {
                printk("Failed to get l2 imu: %d\n", gret);
            }
        } else {
            printk("Failed to fetch l2 imu %d\n", iret);
        }

        if (state != next_state) {
            switch (state) {
                case State::Chilling:
                    break;
                case State::Holding:
                    stop_arm_hold();
                    stop_servo_hold();
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
                case State::Jogging:
                    stop_arm_jog();
                    break;
            }
            switch (next_state) {
                case State::Chilling:
                    break;
                case State::Holding:
                    start_arm_hold();
                    start_servo_hold();
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
                case State::Jogging:
                    start_arm_jog();
                    break;
            }
        }
        state = next_state;
        MovementResult move_result = MovementResult::Ongoing;
        switch (state) {
            case State::Chilling:
                // don't need to do anything
                break;
            case State::Holding:
                move_result = step_arm_hold();
                // servos don't need a step
                break;
            case State::ArmMoving:
                move_result = step_arm();
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
            case State::Jogging:
                printk("Stepping arm jog\n");
                move_result = step_arm_jog();
                break;
        }
        if (state != State::Chilling) {

            if (move_result != MovementResult::Ongoing) {
                printk("ending movement\n");
                switch (state) {
                    case State::Chilling:
                        // don't need to do anything
                        break;
                    case State::Holding:
                        stop_servo_hold();
                        stop_arm_hold();
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
                    case State::Jogging:
                        stop_arm_jog();
                        break;
                }
                state = State::Chilling;
            }
        }
    }
}

namespace CurrentState {
Temperatures temperatures() { return {.link1_temp = 1, .link2_temp = 2, .stm_temp = stored_stm_temp}; }
uint32_t current_iteration() { return iterations; }

Vec3_16 base_imu() { return base_accel; }
Vec3_16 link1_imu() {
    return {
        .x = 0,
        .y = 0,
        .z = 1,
    };
}
Vec3_16 link2_imu() { return l2_accel.to16(); }

} // namespace CurrentState

Vec3_16 Vec3_32::to16() { return {static_cast<int16_t>(x / 1000), static_cast<int16_t>(y / 1000), static_cast<int16_t>(z / 1000)}; }