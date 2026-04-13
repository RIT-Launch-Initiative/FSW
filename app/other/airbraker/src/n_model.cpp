#ifndef AIRBRAKER_BENCHMARK
#include "n_model.hpp"
#include "n_preboost.hpp"
#else
#include "math/matrix.hpp"
#include "n_autocoder_types.h"

namespace NModel {
struct KalmanState {
    float estAltitude;
    float estVelocity;
    float estAcceleration;
    float estBias;
};
void RotateRocketVectorToIMUVector(const NTypes::AccelerometerData& xyz, NTypes::AccelerometerData& out);
} // namespace NModel
#endif

#include "math/matrix.hpp"
#include "n_autocoder_types.h"
#include "quantile_lut_data.h"

#include <cmath>

namespace NModel {
namespace {
constexpr float kCosMaxTilt = 0.8660254037844386F;
constexpr float kDefaultGyroDeltaS = 0.01F;
constexpr float kMaxGyroDeltaS = 0.05F;

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Quaternion {
    float w;
    float x;
    float y;
    float z;
};

float ClampUnit(float v) {
    if (v > 1.0F) {
        return 1.0F;
    }
    if (v < -1.0F) {
        return -1.0F;
    }
    return v;
}

float Dot3(const Vec3& a, const Vec3& b) { return (a.x * b.x) + (a.y * b.y) + (a.z * b.z); }

Vec3 Cross3(const Vec3& a, const Vec3& b) {
    return {
        .x = (a.y * b.z) - (a.z * b.y),
        .y = (a.z * b.x) - (a.x * b.z),
        .z = (a.x * b.y) - (a.y * b.x),
    };
}

Quaternion QuaternionMultiply(const Quaternion& lhs, const Quaternion& rhs) {
    return {
        .w = (lhs.w * rhs.w) - (lhs.x * rhs.x) - (lhs.y * rhs.y) - (lhs.z * rhs.z),
        .x = (lhs.w * rhs.x) + (lhs.x * rhs.w) + (lhs.y * rhs.z) - (lhs.z * rhs.y),
        .y = (lhs.w * rhs.y) - (lhs.x * rhs.z) + (lhs.y * rhs.w) + (lhs.z * rhs.x),
        .z = (lhs.w * rhs.z) + (lhs.x * rhs.y) - (lhs.y * rhs.x) + (lhs.z * rhs.w),
    };
}

Quaternion NormalizeQuaternion(const Quaternion& q) {
    const float normSq = (q.w * q.w) + (q.x * q.x) + (q.y * q.y) + (q.z * q.z);
    if (normSq == 0.0F) {
        return {1.0F, 0.0F, 0.0F, 0.0F};
    }

    const float invNorm = 1.0F / std::sqrt(normSq);
    return {
        .w = q.w * invNorm,
        .x = q.x * invNorm,
        .y = q.y * invNorm,
        .z = q.z * invNorm,
    };
}

Quaternion QuaternionDeltaFromGyro(const NTypes::GyroscopeData& gyro, float deltaSeconds) {
    const float rx = gyro.X * deltaSeconds;
    const float ry = gyro.Y * deltaSeconds;
    const float rz = gyro.Z * deltaSeconds;
    const float rotationMagnitudeSq = (rx * rx) + (ry * ry) + (rz * rz);
    const float rotationMagnitude = std::sqrt(rotationMagnitudeSq);

    if (rotationMagnitude == 0.0F) {
        return {1.0F, 0.0F, 0.0F, 0.0F};
    }

    const float halfAngle = 0.5F * rotationMagnitude;
    const float vectorScale = std::sin(halfAngle) / rotationMagnitude;
    return {
        .w = std::cos(halfAngle),
        .x = rx * vectorScale,
        .y = ry * vectorScale,
        .z = rz * vectorScale,
    };
}

Vec3 RotateVectorByQuaternion(const Quaternion& q, const Vec3& v) {
    const Vec3 u{q.x, q.y, q.z};
    const Vec3 uv = Cross3(u, v);
    const Vec3 uuv = Cross3(u, uv);
    return {
        .x = v.x + (2.0F * ((q.w * uv.x) + uuv.x)),
        .y = v.y + (2.0F * ((q.w * uv.y) + uuv.y)),
        .z = v.z + (2.0F * ((q.w * uv.z) + uuv.z)),
    };
}

const Vec3& InitialRocketZAxisInIMUSpace() {
    static const Vec3 initial = [] {
        NTypes::AccelerometerData zInIMUSpace{};
        RotateRocketVectorToIMUVector({0, 0, 1}, zInIMUSpace);
        return Vec3{zInIMUSpace.X, zInIMUSpace.Y, zInIMUSpace.Z};
    }();
    return initial;
}
} // namespace

const char* GetMatlabLUTName() { return LUT_NAME; }
const char* GetMatlabLUTDate() { return LUT_CREATION_DATE; }

using StateTransitionT = Matrix<4, 4>;

const StateTransitionT state_transition_matrix{{// aka F
                                                KALMAN_STATE_TRANSITION_INITIALIZER}};

const Matrix<2, 4> kalman_output_matrix{{// aka H
                                         KALMAN_OUTPUT_INITIALIZER}};
const Matrix<4, 2> kalman_gain{{KALMAN_GAIN_INITIALIZER}};

static Matrix<4, 1> kalman_state({KALMAN_INITIAL_STATE_INITIALIZER});

static Matrix<2, 1> lastInnovation{{0, 0}};
static Quaternion gyroOrientation{1.0F, 0.0F, 0.0F, 0.0F};
static bool everWentOutOfBounds = false;

Matrix<4, 1> kalmanPredictAndUpdate(const Matrix<4, 1>& state, const float altitudeMeters,
                                    const float verticalAccelerationMS2) {
    Matrix<2, 1> sensorIn = Matrix<2, 1>::Column({altitudeMeters, verticalAccelerationMS2});

    // change via physics model
    Matrix<4, 1> fx = state_transition_matrix * state;

    // change via sensors
    Matrix<2, 1> innovation = sensorIn - (kalman_output_matrix * fx);
    lastInnovation = innovation;

    Matrix<4, 1> correction = kalman_gain * innovation;

    return fx + correction;
}

void FeedKalman(float altitudeMeters, float verticalAccelerationMS2) {
    kalman_state = kalmanPredictAndUpdate(kalman_state, altitudeMeters, verticalAccelerationMS2);
}
KalmanState LastKalmanState() {
    return {
        .estAltitude = kalman_state.Get(0, 0),
        .estVelocity = kalman_state.Get(1, 0),
        .estAcceleration = kalman_state.Get(2, 0),
        .estBias = kalman_state.Get(3, 0),
    };
}
#define CUSTOM_ATMOSPHERE 1
#ifdef CUSTOM_ATMOSPHERE
float AltitudeMetersFromPressureKPa(float kPa) {
    const float x = kPa * 1000.0F;
    float sum = ATMOSPHERE[AUTOGEN_ATMOSPHERE_NUM_COEFFECIENTS - 1];
    for (size_t i = AUTOGEN_ATMOSPHERE_NUM_COEFFECIENTS - 1; i-- > 0;) {
        sum = (sum * x) + ATMOSPHERE[i];
    }
    return sum;
}
#else
float AltitudeMetersFromPressureKPa(float pressure_kpa) {
    float pressure = pressure_kpa * 10;
    float altitude = (1 - powf(pressure / 1013.25F, 0.190284F)) * (float) (145366.45 * 0.3048);
    return altitude;
}
#endif

Matrix<3, 3> expGyro(float w_1, float w_2, float w_3, float t) {
    const float w_1ᵗ = w_1 * t;
    const float w_2ᵗ = w_2 * t;
    const float w_3ᵗ = w_3 * t;

    const float w_1² = w_1ᵗ * w_1ᵗ;
    const float w_2² = w_2ᵗ * w_2ᵗ;
    const float w_3² = w_3ᵗ * w_3ᵗ;
    const float w_1w_2 = w_1ᵗ * w_2ᵗ;
    const float w_1w_3 = w_1ᵗ * w_3ᵗ;
    const float w_2w_3 = w_2ᵗ * w_3ᵗ;

    const float θ² = w_1² + w_2² + w_3²;
    const float θ = std::sqrt(θ²);
    const float s = (θ == 0.0F) ? 1.0F : (std::sin(θ) / θ);
    const float c = (θ == 0.0F) ? 0.0F : ((1.0F - std::cos(θ)) / θ²);

    return Matrix<3, 3>{{
        1.0F - c * (w_2² + w_3²),
        c * w_1w_2 - s * w_3ᵗ,
        c * w_1w_3 + s * w_2ᵗ,
        c * w_1w_2 + s * w_3ᵗ,
        1.0F - c * (w_1² + w_3²),
        c * w_2w_3 - s * w_1ᵗ,
        c * w_1w_3 - s * w_2ᵗ,
        c * w_2w_3 + s * w_1ᵗ,
        1.0F - c * (w_1² + w_2²),
    }};
}

float deg2rad(float d) { return d / 180.0F * 3.14159F; }
float rad2deg(float d) { return d * 180.0F / 3.14159F; }

static uint32_t lastGyroIntegrationMsSinceBoot = 0;
void FeedGyro(uint32_t msSinceBoot, const NTypes::GyroscopeData& gyro) {
    float t = kDefaultGyroDeltaS;
    if (lastGyroIntegrationMsSinceBoot != 0) {
        const int32_t deltaMs = (int32_t) (msSinceBoot - lastGyroIntegrationMsSinceBoot);
        if (deltaMs > 0) {
            t = static_cast<float>(deltaMs) / 1000.F;
            if (t > kMaxGyroDeltaS) {
                t = kMaxGyroDeltaS;
            }
        }
    }
    lastGyroIntegrationMsSinceBoot = msSinceBoot;
    const Quaternion delta = QuaternionDeltaFromGyro(gyro, t);
    gyroOrientation = NormalizeQuaternion(QuaternionMultiply(gyroOrientation, delta));

    const Vec3& initial = InitialRocketZAxisInIMUSpace();
    const Vec3 now = RotateVectorByQuaternion(gyroOrientation, initial);
    const float cosOffStart = ClampUnit(Dot3(initial, now));
    const bool outOfBounds = cosOffStart < kCosMaxTilt;
    everWentOutOfBounds |= outOfBounds;
}

int GetOrientation() { return 0; }
bool EverWentOutOfBounds() { return everWentOutOfBounds; }

#ifdef AIRBRAKER_BENCHMARK
void ResetGyroStateForBenchmark() {
    gyroOrientation = {1.0F, 0.0F, 0.0F, 0.0F};
    everWentOutOfBounds = false;
    lastGyroIntegrationMsSinceBoot = 0;
}
#endif

void FillPacketWithOrientationMatrix(float* arr) {
    const float ww = gyroOrientation.w * gyroOrientation.w;
    const float xx = gyroOrientation.x * gyroOrientation.x;
    const float yy = gyroOrientation.y * gyroOrientation.y;
    const float zz = gyroOrientation.z * gyroOrientation.z;
    const float wx = gyroOrientation.w * gyroOrientation.x;
    const float wy = gyroOrientation.w * gyroOrientation.y;
    const float wz = gyroOrientation.w * gyroOrientation.z;
    const float xy = gyroOrientation.x * gyroOrientation.y;
    const float xz = gyroOrientation.x * gyroOrientation.z;
    const float yz = gyroOrientation.y * gyroOrientation.z;

    arr[0] = ww + xx - yy - zz;
    arr[1] = 2.0F * (xy - wz);
    arr[2] = 2.0F * (xz + wy);
    arr[3] = 2.0F * (xy + wz);
    arr[4] = ww - xx + yy - zz;
    arr[5] = 2.0F * (yz - wx);
    arr[6] = 2.0F * (xz - wy);
    arr[7] = 2.0F * (yz + wx);
    arr[8] = ww - xx - yy + zz;
}

void FillPacketWithKalmanInformation(float* inno, KalmanState& state) {
    inno[0] = lastInnovation.Get(0, 0);
    inno[1] = lastInnovation.Get(1, 0);
    state = LastKalmanState();
}

} // namespace NModel
