#include "common.h"

#include <cstdint>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/shell/shell.h>

int servo_preflight(const struct shell *shell);
// Turn on the servo and lock it in place so that when apogee happens the door stays shut
void servo_at_boost();

struct ServoState {
    uint32_t last_ticks;
};

bool am_upright(const struct device *imu_dec);

enum SweepStrategy {
    Slow,
    Fast,
    Faster,
};

struct Servo {
    struct pwm_dt_spec pwm;
    uint32_t open_pulselen;
    uint32_t closed_pulselen;
    ServoState &state;

    int disconnect() const;
    int open() const;
    int close() const;
    int set_pulse(uint32_t pulse) const;
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

// Face ID and the unit vector that represents its direction
struct FaceAndId {
    PayloadFace id;
    NTypes::AccelerometerData direction;
};
/**
 * Return string representation of a face
 */
const char *string_face(PayloadFace p);

int flip_one_side(const struct device *ina_dev, const Servo &servo, SweepStrategy strat, bool open,
                  bool hold_strong = false);
int init_flip_hw();

int do_flipping_and_pumping(const struct device *imu_dev, const struct device *barom_dev,
                            const struct device *ina_servo, const struct device *ina_pump);
