#include "servo.hpp"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>

FlipServoMotion servo_motions[3] = {0};

constexpr size_t SERVO_PERIOD_NS = 3000000; // 3 ms
const pwm_dt_spec flip_servos[3] = {
    PWM_DT_SPEC_GET(DT_NODELABEL(servo1)),
    PWM_DT_SPEC_GET(DT_NODELABEL(servo2)),
    PWM_DT_SPEC_GET(DT_NODELABEL(servo3)),
};

const gpio_dt_spec flip_servo_enable = GPIO_DT_SPEC_GET(DT_NODELABEL(flip_servo_pwr_en), gpios);

// TODO some of these will be flipped
const uint32_t min_pulses[3] = {
    800,
    1970,
    800,
};

const uint32_t max_pulses[3] = {
    1700,
    800,
    1700,
};

uint32_t current_pulses_us[3] = {min_pulses[0], min_pulses[1], min_pulses[2]};

bool am_active = false;
FlipServoMotionState active_motion_state;
FlipServo active_servo;
uint32_t active_servo_start_position = 0;

// https://forum.arduino.cc/t/map-function-only-integers/335579/2
uint32_t map_openness(uint8_t val, uint32_t min_pulse, uint32_t max_pulse) {
    const int32_t min_in = 0;
    const int32_t max_in = 255;
    int32_t min_out = (int32_t) min_pulse;
    int32_t max_out = (int32_t) max_pulse;

    return (((val - min_in) * (max_out - min_out)) / (max_in - min_in)) + min_out;
}
uint32_t lerp_pulse(uint32_t ts, uint32_t time_range, uint32_t from_us, uint32_t to_us) {
    
    // t = (ts/time_range)
    // return t * (to_us - from_us) + from_us
    printf("  l(%d/%d  to range (%d, %d)  ", ts, time_range, from_us, to_us);
    return ((int32_t)ts * ((int32_t)to_us - (int32_t)from_us)) / (int32_t)time_range + from_us;
}

uint32_t FlipServoMotionState::pulse_length_at_iteration(uint32_t iteration, uint32_t min_pulse, uint32_t max_pulse) {
    if (iteration < iteration_started) {
        return 0; // before starting, dont command the servo
    }
    uint32_t delta = iteration -  iteration_started;
    uint32_t close_pulse = map_openness(motion.closedness, min_pulse, max_pulse);
    uint32_t open_pulse = map_openness(motion.openness, min_pulse, max_pulse);
    if (delta < motion.open_travel_duration) {
        // lerp between active_servo_start_position
        printk("iter: %d/%d %d-%d  %d", delta, motion.open_travel_duration, active_servo_start_position, open_pulse, motion.openness);
        printk("Openning ");
        return lerp_pulse(delta, motion.open_travel_duration, active_servo_start_position, open_pulse);
    } else if (delta < motion.open_travel_duration + motion.open_duration) {
        // holding fast at open position
        printk("Holding ");
        return map_openness(motion.openness, min_pulse, max_pulse);
    } else if (delta < motion.total_duration()) {
        // lerp between open and close position
        int32_t close_delta = delta - motion.open_duration - motion.open_travel_duration;
        printk("iter: %d ", close_delta);
        printk("Closing ");
        return lerp_pulse(close_delta, motion.close_travel_duration, open_pulse, close_pulse);
    } else {
        return 0;
    }
}

void set_flip_pulse_us(FlipServo servoid, uint32_t us) {
    current_pulses_us[servoid] = us;
    pwm_set_dt(&flip_servos[servoid], SERVO_PERIOD_NS, us * 1000);
}

void servo_init() { gpio_pin_configure_dt(&flip_servo_enable, GPIO_OUTPUT_INACTIVE); }

MovementResult step_servo(FlipServo servoid) {
    if (servoid > FlipServo::Servo3) {
        printk("Too high servo");
        return MovementResult::Invalid;
    }
    if (!am_active) {
        printk("Not active\n");
        return MovementResult::Finished;
    }

    uint32_t iteration = CurrentState::current_iteration();
    if (iteration > active_motion_state.iteration_started + active_motion_state.motion.total_duration()) {
        printk("Finished\n");
        return MovementResult::Finished;
    }

    uint32_t pulselen =
        active_motion_state.pulse_length_at_iteration(iteration, min_pulses[active_servo], max_pulses[active_servo]);
    printk("Set puleslen %d\n", (int) pulselen);

    set_flip_pulse_us(active_servo, pulselen);
    return MovementResult::Ongoing;
}
void stop_servo_move(FlipServo servoid) {
    if (servoid > FlipServo::Servo3) {
        printk("Too high servo");
        return;
    }
    set_flip_pulse_us(active_servo, 0);
    printk("Stopping servo move %d\n", servoid);
    gpio_pin_set_dt(&flip_servo_enable, 0);
    am_active = false;
}
void start_servo_move(FlipServo servoid) {
    if (servoid > FlipServo::Servo3) {
        printk("Too high servo");
        return;
    }
    gpio_pin_set_dt(&flip_servo_enable, 1);
    printk("Starting servo move: %d\n", (int) servoid);

    active_servo = servoid;
    active_motion_state.iteration_started = CurrentState::current_iteration();
    printk("Starting iteration: %d\n", active_motion_state.iteration_started);
    active_motion_state.motion = servo_motions[servoid];
    active_servo_start_position = current_pulses_us[servoid];
    am_active = true;
}

uint32_t FlipServoMotion::total_duration() { return open_duration + open_travel_duration + close_travel_duration; }
void set_servo_motion(FlipServo servoid, FlipServoMotion motion) {
    if (servoid > FlipServo::Servo3) {
        printk("Too high servo\n");
    }
    servo_motions[servoid] = motion;
}

namespace CurrentState {
FlipServoMotion servo_motion(FlipServo servoid) { return servo_motions[servoid]; }
} // namespace CurrentState