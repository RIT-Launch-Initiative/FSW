#include "servo.hpp"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>

FlipServoMotion servo_motions[3] = {0};

constexpr size_t SERVO_PERIOD_NS = 3000000; // 3 ms

const gpio_dt_spec flip_servo_enable = GPIO_DT_SPEC_GET(DT_NODELABEL(flip_servo_pwr_en), gpios);
Servo flip_servo1{PWM_DT_SPEC_GET(DT_NODELABEL(servo1)), 1700, 800, 800}; // back
Servo flip_servo2{PWM_DT_SPEC_GET(DT_NODELABEL(servo2)), 800, 1970,  1970}; // back
Servo flip_servo3{PWM_DT_SPEC_GET(DT_NODELABEL(servo3)), 800, 1700, 800};

Servo *flip_servos[3] = {&flip_servo1, &flip_servo2, &flip_servo3};

Servo *servoById(FlipServo servoid) {
    if (servoid == FlipServo::Servo1) {
        return flip_servos[0];
    } else if (servoid == FlipServo::Servo2) {
        return flip_servos[1];
    }
    return flip_servos[2];
}

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
    printk("  l(%d/%d  to range (%d, %d)  ", ts, time_range, from_us, to_us);
    return ((int32_t) ts * ((int32_t) to_us - (int32_t) from_us)) / (int32_t) time_range + from_us;
}

uint32_t FlipServoMotionState::pulse_length_at_iteration(uint32_t iteration, uint32_t min_pulse, uint32_t max_pulse) {
    if (iteration < iteration_started) {
        return 0; // before starting, dont command the servo
    }
    uint32_t delta = iteration - iteration_started;
    uint32_t close_pulse = map_openness(motion.closedness, min_pulse, max_pulse);
    uint32_t open_pulse = map_openness(motion.openness, min_pulse, max_pulse);
    if (delta < motion.open_travel_duration) {
        // lerp between active_servo_start_position
        printk("iter: %d/%d %d-%d  %d", delta, motion.open_travel_duration, active_servo_start_position, open_pulse,
               motion.openness);
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

void servo_init() { gpio_pin_configure_dt(&flip_servo_enable, GPIO_OUTPUT_INACTIVE); }

MovementResult step_servo(FlipServo servoid) {
    Servo *servo = servoById(servoid);

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

    uint32_t pulselen = active_motion_state.pulse_length_at_iteration(iteration, servo->closed_us, servo->open_us);
    printk("Set puleslen %d\n", (int) pulselen);

    int sret = servoById(active_servo)->set_us(pulselen);
    printk(" sret: %d \n", sret);
    return MovementResult::Ongoing;
}
void stop_servo_move(FlipServo servoid) {
    if (servoid > FlipServo::Servo3) {
        printk("Too high servo");
        return;
    }
    servoById(active_servo)->disconnect();
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
    active_servo_start_position = servoById(servoid)->last_us;
    am_active = true;
}

void start_servo_hold() {
    gpio_pin_set_dt(&flip_servo_enable, 1);
    printk("Starting servo hold\n");
    for (auto servo : flip_servos) {
        servo->set_us(servo->last_us); 
    }
}
void stop_servo_hold() {
    printk("Stopping servo hold\n");

    for (auto servo : flip_servos) {
        servo->disconnect();
    }
    gpio_pin_set_dt(&flip_servo_enable, 0);
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
bool flip_en() { return gpio_pin_get_dt(&flip_servo_enable); }
} // namespace CurrentState

int Servo::open() { return set_us(open_us); }

int Servo::close() { return set_us(closed_us); }

int Servo::set_us(uint32_t us) {
    last_us = us;
    printk("S to %u", us);
    return pwm_set_dt(&pwm, SERVO_PERIOD_NS, us * 1000);
}

int Servo::disconnect() { return pwm_set_pulse_dt(&pwm, 0); }
