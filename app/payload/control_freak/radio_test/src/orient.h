#include <array>
#include <cstdint>
#include <zephyr/drivers/pwm.h>

struct ServoState {
    uint32_t last_ticks;
};
struct Servo {
    struct pwm_dt_spec pwm;
    uint32_t open_pulselen;
    uint32_t closed_pulselen;
    ServoState &state;

    int open() const;
    int close() const;
    int set_pulse(uint32_t ticks) const;

    bool was_fully_open() const;
    bool was_fully_closed() const;

};

struct vec3 {
    float x;
    float y;
    float z;
};

enum PayloadFace : uint8_t {
    Face1 = 0,
    Face2 = 1,
    Face3 = 2,
    Upright = 3,
    StandingUp = 4,
    OnItsHead = 5,
    NumFaces = 6,
};

const char *string_face(PayloadFace p);
struct FaceAndId {
    PayloadFace id;
    vec3 direction;
};

PayloadFace find_orientation(const vec3 &me);

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
