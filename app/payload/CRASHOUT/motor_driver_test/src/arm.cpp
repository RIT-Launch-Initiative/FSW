#include "arm.hpp"

#include <zephyr/kernel.h>

static ArmPose current_pose = {1, 2, 3, 4};
static ArmPose target_pose = {5, 6, 7, 8};

void set_arm_target(const ArmPose &pose) { target_pose = pose; }

void set_arm_pose(const ArmPose &pose) { current_pose = pose; }

void start_arm() { printk("Starting arm"); }

void step_arm() {}

void stop_arm() { printk("Stopping arm"); }

namespace CurrentState {
Vec3_16 base_imu() {
    return {
        .x = 0,
        .y = 1,
        .z = 2,
    };
}
Vec3_16 link1_imu() {
    return {
        .x = 10,
        .y = 11,
        .z = 12,
    };
}
Vec3_16 link2_imu() {
    return {
        .x = 20,
        .y = 21,
        .z = 22,
    };
}

ArmPose arm_pose_est() { return current_pose; }
ArmPose arm_pose_target() { return target_pose; }

}; // namespace CurrentState