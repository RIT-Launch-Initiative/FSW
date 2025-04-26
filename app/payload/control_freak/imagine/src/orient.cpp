#include "orient.h"

#include <algorithm>
#include <math.h>
#include <zephyr/drivers/sensor.h>

#define IMU_NODE DT_ALIAS(imu)
static const struct device *imu_dev = DEVICE_DT_GET(IMU_NODE);

int Servo::open() const { return set_pulse(open_pulselen); }
int Servo::close() const { return set_pulse(closed_pulselen); }
int Servo::set_pulse(uint32_t pulse) const {
    state.last_ticks = pulse;
    return pwm_set_pulse_dt(&pwm, pulse);
}

int Servo::disconnect() const { return pwm_set_pulse_dt(&pwm, 0); }

bool Servo::was_fully_open() const { return state.last_ticks == open_pulselen; }
bool Servo::was_fully_closed() const { return state.last_ticks == closed_pulselen; }

float dot(const vec3 &a, const vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

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

PayloadFace find_orientation(const vec3 &me) {
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

    return sims[0].id;
}

int find_vector(vec3 &me) {

    int ret = sensor_sample_fetch(imu_dev);
    if (ret < 0) {
        printk("Failed to fetch imu: %d\n", ret);
        return ret;
    }
    struct sensor_value xyz[3] = {0};
    ret = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, &xyz[0]);
    if (ret < 0) {
        printk("Failed to get imu: %d\n", ret);
        return ret;
    }
    me = {sensor_value_to_float(&xyz[0]), sensor_value_to_float(&xyz[1]), sensor_value_to_float(&xyz[2])};
    printk("[%f, %f, %f]", me.x, me.y, me.z);
    float norm = sqrt(me.x * me.x + me.y * me.y + me.z * me.z);
    me.x /= norm;
    me.y /= norm;
    me.z /= norm;

    return 0;
}