#include <stdint.h>
// operate off of and swap with spi handler to expose information to the spi bus
struct exposed_state {
    // general state
    uint32_t uptime_ms;
    uint8_t status;

    uint8_t system_fault;
    uint8_t motor_fault;

    // things that the pi sets

    uint8_t commanded_joint1_angle; // motor: Shoulder yaw
    uint8_t commanded_joint2_angle; // motor: Shoulder pitch
    uint8_t commanded_joint3_angle; // motor: Elbow pitch
    uint8_t commanded_joint4_angle; // servo: Wrist pitch

    int16_t base_accel_x;
    int16_t base_accel_y;
    int16_t base_accel_z;

    // telemetry
    uint8_t current_joint1_angle; // motor: Shoulder yaw
    uint8_t current_joint2_angle; // motor: Shoulder pitch
    uint8_t current_joint3_angle; // motor: Elbow pitch

    int16_t accel1_x;
    int16_t accel1_y;
    int16_t accel1_z;

    int16_t accel2_y;
    int16_t accel2_x;
    int16_t accel2_z;

    uint8_t m1_voltage;
    uint8_t m1_current;

    uint8_t m2_voltage;
    uint8_t m2_current;

    uint8_t m3_voltage;
    uint8_t m3_current;

    uint32_t uptime_of_last_base_accel_write;
};

enum Command {
    // internal use
    RunClockTick, // not emitted by spi interface, but sent down the same channel to make the main loop do things
    //
    SetJointTargets, // set joint target angles (j1, j2, j3, j4 uint8)

    ReadMotorPower, // returns all 3 motors voltages and currents (m1v, m1c, m2v, m2c, m3v, m3c)
    ReadM1Power,    // Returns m1 voltage and current
    ReadM2Power,    // Returns m2 voltage and current
    ReadM3Power,    // Returns m3 voltage and current
};