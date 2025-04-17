#include "orient.h"

#include <algorithm>
#include <zephyr/drivers/sensor.h>

int Servo::open() const { return set_pulse(open_pulselen); }
int Servo::close() const { return set_pulse(closed_pulselen); }
int Servo::set_pulse(uint32_t pulse) const {
    state.last_ticks = pulse;
    return pwm_set_pulse_dt(&pwm, pulse);
}

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

    for (auto s : sims) {
        printk("%11s: %f\n", string_face(s.id), s.similarity);
    }
    return sims[0].id;
}
