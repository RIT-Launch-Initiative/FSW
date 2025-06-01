#include "flipping.h"

#include "5v_ctrl.h"
#include "common.h"
#include "slow_sensing.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(flipping);

struct ServoState {
    uint32_t last_ticks;
};
struct Servo {
    struct pwm_dt_spec pwm;
    uint32_t open_pulselen;
    uint32_t closed_pulselen;
    ServoState &state;

    int disconnect() const { return pwm_set_pulse_dt(&pwm, 0); }
    int open() const { return set_pulse(open_pulselen); }
    int close() const { return set_pulse(closed_pulselen); }
    int set_pulse(uint32_t pulse) const {
        state.last_ticks = pulse;
        return pwm_set_pulse_dt(&pwm, pulse);
    }

    bool was_fully_open() const { return state.last_ticks == open_pulselen; }
    bool was_fully_closed() const { return state.last_ticks == closed_pulselen; }
};

enum PayloadFace : uint8_t {
    Face1 = 0,
    Face2 = 1,
    Face3 = 2,
    Upright = 3,
    StandingUp = 4,
    OnItsHead = 5,
    NumFaces = 6,
    UnknownFace = 7,
};

const char *string_face(PayloadFace p);
struct FaceAndId {
    PayloadFace id;
    NTypes::AccelerometerData direction;
};

float dot(const NTypes::AccelerometerData &a, const NTypes::AccelerometerData &b) {
    return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
}

const char *string_face(PayloadFace p) {
    switch (p) {
        case PayloadFace::Face1:
            return "Face1";
        case PayloadFace::Face2:
            return "Face2";
        case PayloadFace::Face3:
            return "Face3";
        case PayloadFace::Upright:
            return "Upright";
        case PayloadFace::OnItsHead:
            return "On its head";
        case PayloadFace::StandingUp:
            return "StandingUp";
        default:
            return "Unknown side";
    }
    return "Unknown side";
}

// clang-format off
inline constexpr std::array<FaceAndId, PayloadFace::NumFaces> payload_faces = {
    FaceAndId{Face1, {1, 0, 0}}, 
    FaceAndId{Face2, {0, 1, 0}}, 
    FaceAndId{Face3, {-1, 0, 0}}, 
    FaceAndId{Upright, {0, -1, 0}},  
    FaceAndId{StandingUp, {0, 0, -1}}, 
    FaceAndId{OnItsHead, {0, 0, 1}},
};
// clang-format on

PayloadFace find_orientation(const NTypes::AccelerometerData &me, size_t face_index = 0) {
    struct FaceAndSimilarity {
        PayloadFace id;
        float similarity;
    };
    std::array<FaceAndSimilarity, PayloadFace::NumFaces> sims = {};
    for (int i = 0; i < PayloadFace::NumFaces; i++) {
        sims[i].id = payload_faces[i].id;
        sims[i].similarity = dot(me, payload_faces[i].direction);
    }
    std::sort(sims.begin(), sims.end(),
              [](const FaceAndSimilarity &a, const FaceAndSimilarity &b) { return a.similarity > b.similarity; });

    for (auto s : sims) {
        LOG_INF("%11s: %f", string_face(s.id), (double) s.similarity);
    }
    return sims[face_index].id;
}

static constexpr uint32_t min_pulse = PWM_USEC(800);  //DT_PROP(DT_PARENT(DT_A LIAS(servo1)), min_pulse);
static constexpr uint32_t max_pulse = PWM_USEC(1700); //DT_PROP(DT_PARENT(DT_ALIAS(servo1)), max_pulse);

// false when closed, true when open
ServoState state1{};
ServoState state2{};
ServoState state3{};

static constexpr Servo Servo1{
    .pwm = PWM_DT_SPEC_GET(DT_ALIAS(servo1)),
    .open_pulselen = min_pulse,
    .closed_pulselen = max_pulse,
    .state = state1,
};
static constexpr Servo Servo2{
    .pwm = PWM_DT_SPEC_GET(DT_ALIAS(servo2)),
    .open_pulselen = max_pulse,
    .closed_pulselen = min_pulse,
    .state = state2,
};
static constexpr Servo Servo3{
    .pwm = PWM_DT_SPEC_GET(DT_ALIAS(servo3)),
    .open_pulselen = max_pulse,
    .closed_pulselen = min_pulse,
    .state = state3,
};

const Servo *servos[] = {&Servo1, &Servo2, &Servo3};

int init_flip_hw() {
    rail_item_disable(FiveVoltItem::Pump);
    rail_item_disable(FiveVoltItem::Servos);

    return 0;
}

int get_normed_orientation(const struct device *imu_dev, NTypes::AccelerometerData &vec) {

    int ret = sensor_sample_fetch(imu_dev);
    if (ret < 0) {
        printk("Failed to fetch imu: %d\n", ret);
        return ret;
    }
    struct sensor_value xyz[3] = {0};
    ret = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, &xyz[0]);
    if (ret < 0) {
        LOG_WRN("Failed to get imu: %d\n", ret);
        return ret;
    }
    vec = {sensor_value_to_float(&xyz[0]), sensor_value_to_float(&xyz[1]), sensor_value_to_float(&xyz[2])};
    float norm = sqrt(vec.X * vec.X + vec.Y * vec.Y + vec.Z * vec.Z);
    vec.X /= norm;
    vec.Y /= norm;
    vec.Z /= norm;

    return 0;
}

// Best outcome
#define FLIPPED_AND_RIGHTED 0
// Ok, but need to put in some more work
#define FLIPPED_BUT_NOT_RIGHTED 1

enum SweepStrategy {
    Slow,
    Fast,
};

struct Shunt {
    float current;
    float voltage;
};

constexpr int servo_steps = 120;
Shunt current_log[120] = {};

int flip_one_side(const struct device *ina_dev, const Servo &servo, SweepStrategy strat, float &current_before,
                  float &voltage_before) {

    uint32_t start = servo.closed_pulselen;
    uint32_t end = servo.open_pulselen;
    if (servo.state.last_ticks == servo.open_pulselen) {
        start = servo.open_pulselen;
        end = servo.closed_pulselen;
    };

    int delta = end - start;
    LOG_INF("Moving %d to %d", start, end);
    for (int i = 0; i < servo_steps; i++) {
        float current = 0;
        float voltage = 0;
        int ret = read_ina(ina_dev, current, voltage);
        current_log[i].current = current;
        current_log[i].voltage = voltage;

        int pulse = start + delta * i / servo_steps;

        ret = servo.set_pulse(pulse);
        if (ret < 0) {
            printk("Error %d: failed to set pulse width\n", ret);
        }
        k_msleep(20);
    }
    // LOG_INF("Current, Voltage\n");
    // for (int i = 0; i < servo_steps; i++) {
    // LOG_INF("%.4f, %.4f\n", (double) current_log[i].current, (double) current_log[i].voltage);
    // }
    int ret = servo.set_pulse(end);
    if (ret < 0) {
        LOG_WRN("Error %d: failed to set pulse width\n", ret);
    }
    k_msleep(100);
    ret = servo.disconnect();
    if (ret < 0) {
        LOG_WRN("Error %d: failed to disable servo\n", ret);
        return ret;
    }
    return 0;
}

int try_flipping(const struct device *imu_dev, const struct device *ina_dev, int attempts) {
    rail_item_enable(FiveVoltItem::Servos);
    int ret = set_lsm_sampling(imu_dev, 12); // dont need all that speed here
    if (ret < 0) {
        LOG_WRN("Couldn't set sampling");
    }

    NTypes::AccelerometerData vec = {0};
    PayloadFace last_face = PayloadFace::UnknownFace;
    int attempts_on_this_face = 0;

    for (int i = 0; i < attempts; i++) {
        k_msleep(2000); // just in case we're still wobbling around

        int ret = get_normed_orientation(imu_dev, vec);
        if (ret != 0) {
            LOG_WRN("Couldnt read IMU, trying again: %d", ret);
            continue;
        }

        PayloadFace face = find_orientation(vec);
        LOG_INF("Am on face %s", string_face(face));
        if (face == last_face) {
            attempts_on_this_face++;
            LOG_INF("Haven't moved %d times", attempts_on_this_face);
        } else {
            attempts_on_this_face = 0;
        }
        last_face = face;

        if (face == PayloadFace::OnItsHead || face == PayloadFace::StandingUp) {
            LOG_WRN("Oh its so over");
            continue;
        } else if (face == PayloadFace::Upright) {
            LOG_INF("All good");
            rail_item_disable(FiveVoltItem::Servos);
            return FLIPPED_AND_RIGHTED;
        }

        float current_before = 0;
        float voltage_before = 0;
        float tempC = 0;
        if (face == PayloadFace::Face1) {
            ret = flip_one_side(ina_dev, Servo1, Slow, current_before, voltage_before);
        } else if (face == PayloadFace::Face1) {
            ret = flip_one_side(ina_dev, Servo2, Slow, current_before, voltage_before);
        } else if (face == PayloadFace::Face1) {
            ret = flip_one_side(ina_dev, Servo3, Slow, current_before, voltage_before);
        }
        if (ret != 0) {
            LOG_WRN("Error sweeping servo: %d", ret);
        }
        FlipState fs = FLIP_STATE(face, attempts_on_this_face);
        ret = submit_slowdata(vec, tempC, current_before, voltage_before, fs);
        if (ret < 0) {
            LOG_WRN("Failed to send slowdata");
        }
        LOG_INF("Iteration %d complete", i);
    }
    rail_item_disable(FiveVoltItem::Servos);
    return FLIPPED_BUT_NOT_RIGHTED;
}

int do_flipping_and_pumping(const struct device *imu_dev, const struct device *barom_dev,
                            const struct device *ina_servo, const struct device *ina_pump) {

    // Initial flipping
    int ret = try_flipping(imu_dev, ina_servo, 10);
    if (ret != FLIPPED_AND_RIGHTED) {
        LOG_INF("Bad news, we'll worry about this later");
    }
    while (true) {
        k_msleep(1000);
    }

    return -ENOTSUP;
}
