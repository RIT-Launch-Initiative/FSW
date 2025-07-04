#include "flipping.h"

#include "5v_ctrl.h"
#include "f_core/utils/linear_fit.hpp"
#include "pump.h"
#include "slow_sensing.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(flipping);

int Servo::disconnect() const { return pwm_set_pulse_dt(&pwm, 0); }
int Servo::open() const { return set_pulse(open_pulselen); }
int Servo::close() const { return set_pulse(closed_pulselen); }
int Servo::set_pulse(uint32_t pulse) const {
    state.last_ticks = pulse;
    return pwm_set_pulse_dt(&pwm, pulse);
}

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

/**
 * Sort and get the orientation that the payload is facing
 * @param me normalized vector representing your orientation (gravity vector)
 * @param face_index index from most similar to least similar if most similar isnt working, maybe try second most similar
 */
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

    // for (auto s : sims) {
    // LOG_INF("%11s: %f", string_face(s.id), (double) s.similarity);
    // }
    return sims[face_index].id;
}

static constexpr uint32_t min_pulse = PWM_USEC(800);  //DT_PROP(DT_PARENT(DT_A LIAS(servo1)), min_pulse);
static constexpr uint32_t max_pulse = PWM_USEC(1700); //DT_PROP(DT_PARENT(DT_ALIAS(servo1)), max_pulse);

// States of the servos, initted in init_hw
ServoState state1{};
ServoState state2{};
ServoState state3{};

static constexpr Servo Servo1{
    .pwm = PWM_DT_SPEC_GET(DT_ALIAS(servo1)),
    .open_pulselen = PWM_USEC(930),
    .closed_pulselen = PWM_USEC(2010),
    .state = state1,
};
static constexpr Servo Servo2{
    .pwm = PWM_DT_SPEC_GET(DT_ALIAS(servo2)),
    .open_pulselen = PWM_USEC(1900),
    .closed_pulselen = PWM_USEC(985),
    .state = state2,
};
static constexpr Servo Servo3{
    .pwm = PWM_DT_SPEC_GET(DT_ALIAS(servo3)),
    .open_pulselen = PWM_USEC(1940),
    .closed_pulselen = PWM_USEC(800),
    .state = state3,
};

const Servo *servos[] = {&Servo1, &Servo2, &Servo3};

int init_flip_hw() {
    rail_item_disable(FiveVoltItem::Pump);
    rail_item_disable(FiveVoltItem::Servos);

    state1.last_ticks = Servo1.closed_pulselen;
    state2.last_ticks = Servo2.closed_pulselen;
    state3.last_ticks = Servo3.closed_pulselen;

    return 0;
}

void servo_at_boost() {
    rail_item_enable(FiveVoltItem::Servos);
    Servo2.close();
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

static constexpr float servo_current_cutoff = 2.4;
constexpr int servo_steps = 120;

// #define CURRENT_LOG
#ifdef CURRENT_LOG
struct Shunt {
    float current;
    float voltage;
};

Shunt current_log[120] = {};
#endif
CMovingAverage<float, 5> filter{0};

int flip_one_side(const struct device *ina_dev, const Servo &servo, SweepStrategy strat, bool open, bool hold_strong) {
    int msec = 20;
    if (strat == SweepStrategy::Fast) {
        msec = 5;
    } else if (strat == SweepStrategy::Faster) {
        msec = 2;
    }
    uint32_t start = servo.state.last_ticks;
    uint32_t end = servo.open_pulselen;
    if (!open) {
        end = servo.closed_pulselen;
    };

    filter.Fill(0);

    int delta = end - start;
    LOG_INF("Moving %d to %d", start, end);
    for (int i = 0; i < servo_steps; i++) {
        float current = 0;
        float voltage = 0;
        int ret = read_ina(ina_dev, voltage, current);

        filter.Feed(current);
        if (filter.Avg() > servo_current_cutoff) {
            LOG_WRN("Over currented");
            servo.disconnect();
            break;
        }
#ifdef CURRENT_LOG
        current_log[i].current = current;
        current_log[i].voltage = filter.Avg();
#endif
        int pulse = start + delta * i / servo_steps;

        ret = servo.set_pulse(pulse);
        if (ret < 0) {
            printk("Error %d: failed to set pulse width\n", ret);
        }
        k_msleep(msec);
    }
    k_msleep(100); // Wait to settle

#ifdef CURRENT_LOG
    LOG_INF("Current, Voltage\n");
    for (int i = 0; i < servo_steps; i++) {
        LOG_INF("%.4f %.4f", (double) current_log[i].current, (double) current_log[i].voltage);
        k_msleep(1);
    }
#endif
    if (!hold_strong) {
        int ret = servo.disconnect();
        if (ret < 0) {
            LOG_WRN("Error %d: failed to disable servo\n", ret);
            return ret;
        }
    }
    return 0;
}

bool am_upright(const struct device *imu_dev) {
    NTypes::AccelerometerData vec;
    int ret = get_normed_orientation(imu_dev, vec);
    if (ret != 0) {
        LOG_WRN("Couldnt read IMU: %d", ret);
    }
    PayloadFace face = find_orientation(vec);
    return face == PayloadFace::Upright;
}

int try_flipping(const struct device *imu_dev, const struct device *ina_dev, int attempts, FlightState flight_state) {
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

        float tempC = 0;

        int index = face;
        SweepStrategy strategy = SweepStrategy::Slow;
        if (attempts_on_this_face > 8) {
            strategy = SweepStrategy::Faster;
        } else if (attempts_on_this_face > 3) {
            strategy = SweepStrategy::Fast;
        }
        ret = flip_one_side(ina_dev, *servos[index], strategy, true);
        if (ret != 0) {
            LOG_WRN("Error sweeping servo: %d", ret);
        }

        float volts = 0;
        float amps = 0;
        read_ina(ina_dev, volts, amps);
        // Return
        flip_one_side(ina_dev, *servos[index], Slow, false);

        FlipState fs = FLIP_STATE(face, attempts_on_this_face);
        small_orientation snorm = minify_orientation(vec);
        ret = submit_slowdata(snorm, tempC, amps, volts, fs, flight_state);
        if (ret < 0) {
            LOG_WRN("Failed to send slowdata");
        }

        LOG_INF("Iteration %d complete", i);
    }
    rail_item_disable(FiveVoltItem::Servos);
    return FLIPPED_BUT_NOT_RIGHTED;
}

int measure_and_send_data(const struct device *imu_dev, const struct device *bmp_dev, const struct device *ina_servo,
                          const FlightState &flight_state) {
    NTypes::AccelerometerData vec = {0};
    NTypes::GyroscopeData _gyro = {0};

    read_imu(imu_dev, vec, _gyro);
    NTypes::AccelerometerData normed = normalize(vec);
    small_orientation snormed = minify_orientation(normed);
    float temp = 1.0;
    float press = 2;
    read_barom(bmp_dev, temp, press);

    float current = 0;
    float volts = 0;
    int ret = read_ina(ina_servo, volts, current);
    if (ret != 0) {
        LOG_WRN("Couldnt read ina: %d", ret);
    }
    ret = submit_slowdata(snormed, temp, current, volts, FLIP_STATE_NOT_TRYING, flight_state);
    if (ret != 0) {
        LOG_WRN("Couldn submit slowdata: %d", ret);
    }

    ret = submit_horus_data(temp, press, volts, snormed, flight_state);
    if (ret != 0) {
        LOG_WRN("Couldn submit horus: %d", ret);
    }
    return 0;
}
extern void set_radio_battery_save();

K_TIMER_DEFINE(radio_chillout_tmr, NULL, NULL);

int do_flipping_and_pumping(const struct device *imu_dev, const struct device *barom_dev,
                            const struct device *ina_servo, const struct device *ina_pump) {

    // Initial flipping
    static constexpr int initial_flipping_attempts = 10;
    int ret = try_flipping(imu_dev, ina_servo, initial_flipping_attempts, FlightState::InitialRoll);
    if (ret != FLIPPED_AND_RIGHTED) {
        LOG_INF("Bad news, we'll worry about this later");
    }
    LOG_INF("Low Power");
    set_lsm_sampling(imu_dev, 1);

    rail_item_enable(FiveVoltItem::Servos);
    flip_one_side(ina_servo, Servo1, SweepStrategy::Fast, true, true);
    k_msleep(1000);
    flip_one_side(ina_servo, Servo2, SweepStrategy::Fast, true, false);
    k_msleep(1000);
    flip_one_side(ina_servo, Servo3, SweepStrategy::Fast, true, true);
    k_msleep(1000);

    Servo2.disconnect();

    static constexpr int initial_inflation_attempts = 20;

    FlightState flight_state = FlightState::InitialPump;
    for (int attempt_number = 0; attempt_number < initial_inflation_attempts; attempt_number++) {

        measure_and_send_data(imu_dev, barom_dev, ina_servo, flight_state);

        int ret = attempt_inflation_iteration(ina_pump);
        if (ret == PUMP_CURRENT_END) {
            LOG_INF("Ended in current shutoff, go less often");
            break;
        }
        k_msleep(PUMP_DUTY_OFF_MS_INITITAL);
    }
    LOG_INF("Finished initial pump");
    auto give_up = []() {
        // Give up
        Servo1.disconnect();
        Servo3.disconnect();
        set_radio_battery_save();
    };

    int not_upright_count = 0;
    k_timer_start(&radio_chillout_tmr, K_MINUTES(30), K_FOREVER);
    while (true) {
        if (k_timer_status_get(&radio_chillout_tmr) > 0) {
            LOG_INF("Giving up since its been so long");
            give_up();
        }
        if (!am_upright(imu_dev) && not_upright_count <= 10) {
            not_upright_count++;
            LOG_WRN("Not upright part %d/10", not_upright_count);
            if (not_upright_count > 10) {
                LOG_WRN("Fell over, give up");
                give_up();
            }
        }
        measure_and_send_data(imu_dev, barom_dev, ina_servo, flight_state);
        attempt_inflation_iteration(ina_pump);
        k_msleep(PUMP_DUTY_OFF_MS);
    }

    return -ENOTSUP;
}

int servo_preflight(const struct shell *shell) {
    rail_item_enable(FiveVoltItem::Servos);
    for (int i = 0; i < 3; i++) {
        shell_print(shell, "Servo %d Open", i + 1);
        servos[i]->open();
        k_msleep(1000);
        servos[i]->disconnect();
    }
    for (int i = 0; i < 3; i++) {
        shell_print(shell, "Servo %d Close", i + 1);
        servos[i]->close();
        k_msleep(1000);
        servos[i]->disconnect();
    }
    rail_item_disable(FiveVoltItem::Servos);

    return 0;
}

int cmd_open(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        shell_error(shell, "SPecify which serv 1-3");
        return -1;
    }
    int servo = atoi(argv[1]);
    if (servo < 1 || servo > 3) {
        shell_error(shell, "Bad servo arg");
        return -1;
    }

    rail_item_enable(FiveVoltItem::Servos);
    shell_print(shell, "Opening %d", servo);
    servos[servo - 1]->open();
    k_msleep(1000);
    servos[servo - 1]->disconnect();
    rail_item_disable(FiveVoltItem::Servos);
    return 0;
}

int cmd_close(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        shell_error(shell, "SPecify which serv 1-3");
        return -1;
    }
    int servo = atoi(argv[1]);
    if (servo < 1 || servo > 3) {
        shell_error(shell, "Bad servo arg");
        return -1;
    }

    rail_item_enable(FiveVoltItem::Servos);
    shell_print(shell, "Opening %d", servo);
    servos[servo - 1]->close();
    k_msleep(1000);
    servos[servo - 1]->disconnect();
    rail_item_disable(FiveVoltItem::Servos);
    return 0;
}
int cmd_zeroall(const struct shell *shell, size_t argc, char **argv) {
    rail_item_enable(FiveVoltItem::Servos);
    shell_print(shell, "Closing 1");
    Servo1.close();
    k_msleep(1000);
    Servo1.disconnect();
    shell_print(shell, "Closing 2");
    Servo2.close();
    k_msleep(1000);
    Servo2.disconnect();
    shell_print(shell, "Closing 3");
    Servo3.close();
    k_msleep(1000);
    Servo3.disconnect();
    rail_item_disable(FiveVoltItem::Servos);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(servo_subcmds, SHELL_CMD(zero, NULL, "Zero all servos", cmd_zeroall),
                               SHELL_CMD(open, NULL, "Open servo", cmd_open),
                               SHELL_CMD(close, NULL, "Clos servo", cmd_close), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(servo, &servo_subcmds, "Servo Commands", NULL);
