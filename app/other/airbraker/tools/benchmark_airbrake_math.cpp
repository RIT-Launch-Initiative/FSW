#include "../src/math/c_manualmatrix.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <vector>

namespace {

using Matrix3 = ManualMatrix<3, 3, float>;
using Vector3 = ManualMatrix<3, 1, float>;
using ArrayVec3 = std::array<float, 3>;

struct Quaternion {
    float w;
    float x;
    float y;
    float z;
};

constexpr std::array<float, 7> kAtmosphere = {
    1.2345e2F, -1.876e-2F, 3.4e-6F, -5.1e-10F, 7.2e-14F, -3.3e-18F, 6.8e-23F,
};

constexpr std::array<std::array<float, 3>, 3> kImuToRocket = {{
    {0.9961947F, -0.0121320F, 0.0864101F},
    {0.0151344F, 0.9993007F, -0.0336292F},
    {-0.0858317F, 0.0349465F, 0.9956955F},
}};

template <typename T>
inline void DoNotOptimize(T const& value) {
    asm volatile("" : : "g"(value) : "memory");
}

float ClampUnit(float v) { return std::max(-1.0F, std::min(1.0F, v)); }

float Dot3(const Vector3& a, const Vector3& b) {
    return (a.Get(0, 0) * b.Get(0, 0)) + (a.Get(1, 0) * b.Get(1, 0)) + (a.Get(2, 0) * b.Get(2, 0));
}

float Dot3(const ArrayVec3& a, const ArrayVec3& b) { return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]); }

ArrayVec3 Cross3(const ArrayVec3& a, const ArrayVec3& b) {
    return {
        (a[1] * b[2]) - (a[2] * b[1]),
        (a[2] * b[0]) - (a[0] * b[2]),
        (a[0] * b[1]) - (a[1] * b[0]),
    };
}

std::array<float, 3> RotateImuToRocket(const std::array<float, 3>& v) {
    std::array<float, 3> out{};
    for (int r = 0; r < 3; ++r) {
        out[r] = (kImuToRocket[r][0] * v[0]) + (kImuToRocket[r][1] * v[1]) + (kImuToRocket[r][2] * v[2]);
    }
    return out;
}

std::array<float, 3> RotateRocketToImu(const std::array<float, 3>& v) {
    std::array<float, 3> out{};
    for (int r = 0; r < 3; ++r) {
        out[r] = (kImuToRocket[0][r] * v[0]) + (kImuToRocket[1][r] * v[1]) + (kImuToRocket[2][r] * v[2]);
    }
    return out;
}

Matrix3 ExpGyroOriginal(float w1, float w2, float w3, float t) {
    w1 *= t;
    w2 *= t;
    w3 *= t;

    const float w1_2 = w1 * w1;
    const float w2_2 = w2 * w2;
    const float w3_2 = w3 * w3;
    const float w1w2 = w1 * w2;
    const float w1w3 = w1 * w3;
    const float w2w3 = w2 * w3;

    Matrix3 A{{
        0,
        -w3,
        w2,
        w3,
        0,
        -w1,
        -w2,
        w1,
        0,
    }};
    Matrix3 A2{{
        -(w2_2 + w3_2),
        w1w2,
        w1w3,
        w1w2,
        -(w1_2 + w3_2),
        w2w3,
        w1w3,
        w2w3,
        -(w1_2 + w2_2),
    }};

    const float normSq = w1_2 + w2_2 + w3_2;
    const float norm = std::sqrt(normSq);
    const float s = (norm == 0.0F) ? 1.0F : (std::sin(norm) / norm);
    const float c = (norm == 0.0F) ? 0.0F : ((1.0F - std::cos(norm)) / normSq);

    return Matrix3::Identity() + (A * s) + (A2 * c);
}

Matrix3 ExpGyroNew(float w1, float w2, float w3, float t) {
    const float w₁ᵗ = w1 * t;
    const float w₂ᵗ = w2 * t;
    const float w₃ᵗ = w3 * t;

    const float w₁² = w₁ᵗ * w₁ᵗ;
    const float w₂² = w₂ᵗ * w₂ᵗ;
    const float w₃² = w₃ᵗ * w₃ᵗ;
    const float w₁w₂ = w₁ᵗ * w₂ᵗ;
    const float w₁w₃ = w₁ᵗ * w₃ᵗ;
    const float w₂w₃ = w₂ᵗ * w₃ᵗ;

    const float θ² = w₁² + w₂² + w₃²;
    const float θ = std::sqrt(θ²);
    const float s = (θ == 0.0F) ? 1.0F : (std::sin(θ) / θ);
    const float c = (θ == 0.0F) ? 0.0F : ((1.0F - std::cos(θ)) / θ²);

    return Matrix3{{
        1.0F - c * (w₂² + w₃²),
        c * w₁w₂ - s * w₃ᵗ,
        c * w₁w₃ + s * w₂ᵗ,
        c * w₁w₂ + s * w₃ᵗ,
        1.0F - c * (w₁² + w₃²),
        c * w₂w₃ - s * w₁ᵗ,
        c * w₁w₃ - s * w₂ᵗ,
        c * w₂w₃ + s * w₁ᵗ,
        1.0F - c * (w₁² + w₂²),
    }};
}

float AltitudeOriginal(float kPa) {
    const float x = kPa * 1000.0F;
    float sum = 0.0F;
    float xn = x;
    for (std::size_t i = 1; i < kAtmosphere.size(); ++i) {
        sum += kAtmosphere[i] * xn;
        xn *= x;
    }
    return sum + kAtmosphere[0];
}

float AltitudeNew(float kPa) {
    const float x = kPa * 1000.0F;
    float sum = kAtmosphere[kAtmosphere.size() - 1];
    for (std::size_t i = kAtmosphere.size() - 1; i-- > 0;) {
        sum = (sum * x) + kAtmosphere[i];
    }
    return sum;
}

Quaternion QuaternionMultiply(const Quaternion& lhs, const Quaternion& rhs) {
    return {
        (lhs.w * rhs.w) - (lhs.x * rhs.x) - (lhs.y * rhs.y) - (lhs.z * rhs.z),
        (lhs.w * rhs.x) + (lhs.x * rhs.w) + (lhs.y * rhs.z) - (lhs.z * rhs.y),
        (lhs.w * rhs.y) - (lhs.x * rhs.z) + (lhs.y * rhs.w) + (lhs.z * rhs.x),
        (lhs.w * rhs.z) + (lhs.x * rhs.y) - (lhs.y * rhs.x) + (lhs.z * rhs.w),
    };
}

Quaternion NormalizeQuaternion(const Quaternion& q) {
    const float normSq = (q.w * q.w) + (q.x * q.x) + (q.y * q.y) + (q.z * q.z);
    if (normSq == 0.0F) {
        return {1.0F, 0.0F, 0.0F, 0.0F};
    }

    const float invNorm = 1.0F / std::sqrt(normSq);
    return {q.w * invNorm, q.x * invNorm, q.y * invNorm, q.z * invNorm};
}

Quaternion QuaternionDeltaFromGyro(float gx, float gy, float gz, float dt) {
    const float rx = gx * dt;
    const float ry = gy * dt;
    const float rz = gz * dt;
    const float rotationMagnitudeSq = (rx * rx) + (ry * ry) + (rz * rz);
    const float rotationMagnitude = std::sqrt(rotationMagnitudeSq);

    if (rotationMagnitude == 0.0F) {
        return {1.0F, 0.0F, 0.0F, 0.0F};
    }

    const float halfAngle = 0.5F * rotationMagnitude;
    const float vectorScale = std::sin(halfAngle) / rotationMagnitude;
    return {std::cos(halfAngle), rx * vectorScale, ry * vectorScale, rz * vectorScale};
}

ArrayVec3 RotateVectorByQuaternion(const Quaternion& q, const ArrayVec3& v) {
    const ArrayVec3 u{q.x, q.y, q.z};
    const ArrayVec3 uv = Cross3(u, v);
    const ArrayVec3 uuv = Cross3(u, uv);
    return {
        v[0] + (2.0F * ((q.w * uv[0]) + uuv[0])),
        v[1] + (2.0F * ((q.w * uv[1]) + uuv[1])),
        v[2] + (2.0F * ((q.w * uv[2]) + uuv[2])),
    };
}

struct GyroStepResult {
    Matrix3 orientation;
    bool outOfBounds;
};

struct QuaternionGyroStepResult {
    Quaternion orientation;
    bool outOfBounds;
};

GyroStepResult FeedGyroOriginal(Matrix3 orientation, float gx, float gy, float gz, float dt) {
    const Matrix3 eAT = ExpGyroOriginal(gx, gy, gz, dt);
    orientation = orientation * eAT;

    const auto zInImu = RotateRocketToImu({0.0F, 0.0F, 1.0F});
    const Vector3 initial{{zInImu[0], zInImu[1], zInImu[2]}};
    const Vector3 now = orientation * initial;
    const auto rotated = RotateImuToRocket({now.Get(0, 0), now.Get(1, 0), now.Get(2, 0)});

    (void) rotated;

    const float initialNorm =
        std::sqrt((initial.Get(0, 0) * initial.Get(0, 0)) + (initial.Get(1, 0) * initial.Get(1, 0)) +
                  (initial.Get(2, 0) * initial.Get(2, 0)));
    const Vector3 normedNow = initial * (1.0F / initialNorm);
    const float dot = (initial.Get(0, 0) * normedNow.Get(0, 0)) + (initial.Get(1, 0) * normedNow.Get(1, 0)) +
                      (initial.Get(2, 0) * normedNow.Get(2, 0));
    const float offStart = std::acos(dot);
    return {orientation, offStart > (30.0F * 3.14159F / 180.0F)};
}

QuaternionGyroStepResult FeedGyroNew(Quaternion orientation, float gx, float gy, float gz, float dt) {
    constexpr float kCosMaxTilt = 0.8660254037844386F;
    static const ArrayVec3 initial = RotateRocketToImu({0.0F, 0.0F, 1.0F});

    const Quaternion delta = QuaternionDeltaFromGyro(gx, gy, gz, dt);
    orientation = NormalizeQuaternion(QuaternionMultiply(orientation, delta));

    const ArrayVec3 now = RotateVectorByQuaternion(orientation, initial);
    const float cosOffStart = ClampUnit(Dot3(initial, now));
    return {orientation, cosOffStart < kCosMaxTilt};
}

struct Stats {
    double minNs;
    double meanNs;
};

template <typename Fn>
Stats RunBenchmark(const char* name, Fn&& fn, std::size_t iterations, std::size_t rounds) {
    std::vector<double> samples;
    samples.reserve(rounds);
    for (std::size_t round = 0; round < rounds; ++round) {
        const auto start = std::chrono::steady_clock::now();
        fn(iterations);
        const auto end = std::chrono::steady_clock::now();
        const double elapsedNs = std::chrono::duration<double, std::nano>(end - start).count();
        samples.push_back(elapsedNs / static_cast<double>(iterations));
    }

    const double minNs = *std::min_element(samples.begin(), samples.end());
    const double meanNs = std::accumulate(samples.begin(), samples.end(), 0.0) / static_cast<double>(samples.size());
    std::printf("%-24s min %.2f ns/op mean %.2f ns/op\n", name, minNs, meanNs);
    return {minNs, meanNs};
}

} // namespace

int main() {
    constexpr std::size_t kIterations = 1'000'000;
    constexpr std::size_t kRounds = 8;

    volatile float sink = 0.0F;
    volatile bool sinkBool = false;

    for (std::size_t i = 0; i < 10000; ++i) {
        const float gx = 0.02F + static_cast<float>(i % 11) * 0.004F;
        const float gy = -0.03F + static_cast<float>(i % 7) * 0.005F;
        const float gz = 0.01F + static_cast<float>(i % 13) * 0.003F;
        sink += ExpGyroOriginal(gx, gy, gz, 0.01F).Get(0, 0);
        sink += ExpGyroNew(gx, gy, gz, 0.01F).Get(0, 0);
        sink += AltitudeOriginal(85.0F + static_cast<float>(i % 50) * 0.1F);
        sink += AltitudeNew(85.0F + static_cast<float>(i % 50) * 0.1F);
    }

    std::puts("Airbraker math benchmark");
    std::printf("iterations=%zu rounds=%zu\n\n", kIterations, kRounds);

    const auto expOriginal = RunBenchmark(
        "expGyro original",
        [&](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                const float gx = 0.02F + static_cast<float>(i % 11) * 0.004F;
                const float gy = -0.03F + static_cast<float>(i % 7) * 0.005F;
                const float gz = 0.01F + static_cast<float>(i % 13) * 0.003F;
                sink += ExpGyroOriginal(gx, gy, gz, 0.01F).Get(0, 0);
            }
            DoNotOptimize(sink);
        },
        kIterations, kRounds);

    const auto expNew = RunBenchmark(
        "expGyro new",
        [&](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                const float gx = 0.02F + static_cast<float>(i % 11) * 0.004F;
                const float gy = -0.03F + static_cast<float>(i % 7) * 0.005F;
                const float gz = 0.01F + static_cast<float>(i % 13) * 0.003F;
                sink += ExpGyroNew(gx, gy, gz, 0.01F).Get(0, 0);
            }
            DoNotOptimize(sink);
        },
        kIterations, kRounds);

    const auto altOriginal = RunBenchmark(
        "altitude original",
        [&](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                sink += AltitudeOriginal(80.0F + static_cast<float>(i % 1000) * 0.02F);
            }
            DoNotOptimize(sink);
        },
        kIterations, kRounds);

    const auto altNew = RunBenchmark(
        "altitude new",
        [&](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                sink += AltitudeNew(80.0F + static_cast<float>(i % 1000) * 0.02F);
            }
            DoNotOptimize(sink);
        },
        kIterations, kRounds);

    const auto feedOriginal = RunBenchmark(
        "feedGyro original",
        [&](std::size_t iterations) {
            Matrix3 orientation = Matrix3::Identity();
            bool everOut = false;
            for (std::size_t i = 0; i < iterations; ++i) {
                const float gx = 0.02F + static_cast<float>(i % 11) * 0.004F;
                const float gy = -0.03F + static_cast<float>(i % 7) * 0.005F;
                const float gz = 0.01F + static_cast<float>(i % 13) * 0.003F;
                const auto result = FeedGyroOriginal(orientation, gx, gy, gz, 0.01F);
                orientation = result.orientation;
                everOut |= result.outOfBounds;
            }
            sink += orientation.Get(0, 0);
            sinkBool ^= everOut;
            DoNotOptimize(sink);
            DoNotOptimize(sinkBool);
        },
        kIterations, kRounds);

    const auto feedNew = RunBenchmark(
        "feedGyro new",
        [&](std::size_t iterations) {
            Quaternion orientation{1.0F, 0.0F, 0.0F, 0.0F};
            bool everOut = false;
            for (std::size_t i = 0; i < iterations; ++i) {
                const float gx = 0.02F + static_cast<float>(i % 11) * 0.004F;
                const float gy = -0.03F + static_cast<float>(i % 7) * 0.005F;
                const float gz = 0.01F + static_cast<float>(i % 13) * 0.003F;
                const auto result = FeedGyroNew(orientation, gx, gy, gz, 0.01F);
                orientation = result.orientation;
                everOut |= result.outOfBounds;
            }
            sink += orientation.w;
            sinkBool ^= everOut;
            DoNotOptimize(sink);
            DoNotOptimize(sinkBool);
        },
        kIterations, kRounds);

    std::puts("\nSpeedups based on best run");
    std::printf("expGyro:        %.2fx\n", expOriginal.minNs / expNew.minNs);
    std::printf("altitude:       %.2fx\n", altOriginal.minNs / altNew.minNs);
    std::printf("feedGyro step:  %.2fx\n", feedOriginal.minNs / feedNew.minNs);
    std::printf("\nignore sinks: %f %d\n", sink, static_cast<int>(sinkBool));
    return 0;
}
