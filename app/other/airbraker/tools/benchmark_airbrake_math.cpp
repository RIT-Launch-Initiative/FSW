#include "../src/math/c_manualmatrix.hpp"

#include "quantile_lut_data.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <vector>

constexpr float ATMOSPHERE[] = {AUTOGEN_ATMOSPHERE_COEFFICIENTS};

#define AIRBRAKER_BENCHMARK 1
#include "../src/n_model.cpp"

namespace {

using Matrix3 = ManualMatrix<3, 3, float>;
using Vector3 = ManualMatrix<3, 1, float>;

struct BenchQuaternion {
    float w;
    float x;
    float y;
    float z;
};

template <typename T>
inline void DoNotOptimize(const T& value)
{
    asm volatile("" : : "g"(value) : "memory");
}

BenchQuaternion QuaternionMultiply(const BenchQuaternion& lhs, const BenchQuaternion& rhs)
{
    return {
        .w = (lhs.w * rhs.w) - (lhs.x * rhs.x) - (lhs.y * rhs.y) - (lhs.z * rhs.z),
        .x = (lhs.w * rhs.x) + (lhs.x * rhs.w) + (lhs.y * rhs.z) - (lhs.z * rhs.y),
        .y = (lhs.w * rhs.y) - (lhs.x * rhs.z) + (lhs.y * rhs.w) + (lhs.z * rhs.x),
        .z = (lhs.w * rhs.z) + (lhs.x * rhs.y) - (lhs.y * rhs.x) + (lhs.z * rhs.w),
    };
}

BenchQuaternion QuaternionConjugate(const BenchQuaternion& q)
{
    return {.w = q.w, .x = -q.x, .y = -q.y, .z = -q.z};
}

std::array<float, 3> RotateVectorByQuaternion(const BenchQuaternion& q, const std::array<float, 3>& v)
{
    const BenchQuaternion p{0.0F, v[0], v[1], v[2]};
    const BenchQuaternion qp = QuaternionMultiply(q, p);
    const BenchQuaternion rotated = QuaternionMultiply(qp, QuaternionConjugate(q));
    return {rotated.x, rotated.y, rotated.z};
}

std::array<float, 3> RotateRocketToImu(const std::array<float, 3>& v)
{
    static constexpr BenchQuaternion kImuToRocket{AUTOGEN_IMU_TO_ROCKET_QUAT_INITIALIZER};
    return RotateVectorByQuaternion(QuaternionConjugate(kImuToRocket), v);
}

float ClampUnit(float v)
{
    return std::max(-1.0F, std::min(1.0F, v));
}

float Dot3(const Vector3& a, const Vector3& b)
{
    return (a.Get(0, 0) * b.Get(0, 0)) + (a.Get(1, 0) * b.Get(1, 0)) + (a.Get(2, 0) * b.Get(2, 0));
}

Matrix3 ExpGyroOriginal(float w1, float w2, float w3, float t)
{
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
        0, -w3, w2,
        w3, 0, -w1,
        -w2, w1, 0,
    }};
    Matrix3 A2{{
        -(w2_2 + w3_2), w1w2, w1w3,
        w1w2, -(w1_2 + w3_2), w2w3,
        w1w3, w2w3, -(w1_2 + w2_2),
    }};

    const float normSq = w1_2 + w2_2 + w3_2;
    const float norm = std::sqrt(normSq);
    const float s = (norm == 0.0F) ? 1.0F : (std::sin(norm) / norm);
    const float c = (norm == 0.0F) ? 0.0F : ((1.0F - std::cos(norm)) / normSq);

    return Matrix3::Identity() + (A * s) + (A2 * c);
}

float AltitudeOriginal(float kPa)
{
    const float x = kPa * 1000.0F;
    float sum = 0.0F;
    float xn = x;
    for (size_t i = 1; i < AUTOGEN_ATMOSPHERE_NUM_COEFFECIENTS; ++i)
    {
        sum += ATMOSPHERE[i] * xn;
        xn *= x;
    }
    return sum + ATMOSPHERE[0];
}

struct GyroStepResult {
    Matrix3 orientation;
    bool outOfBounds;
};

GyroStepResult FeedGyroOriginal(Matrix3 orientation, float gx, float gy, float gz, float dt)
{
    const Matrix3 eAT = ExpGyroOriginal(gx, gy, gz, dt);
    orientation = orientation * eAT;

    const auto zInImu = RotateRocketToImu({0.0F, 0.0F, 1.0F});
    const Vector3 initial{{zInImu[0], zInImu[1], zInImu[2]}};
    const Vector3 now = orientation * initial;

    const float initialNorm = std::sqrt((initial.Get(0, 0) * initial.Get(0, 0)) +
                                        (initial.Get(1, 0) * initial.Get(1, 0)) +
                                        (initial.Get(2, 0) * initial.Get(2, 0)));
    const Vector3 normedNow = initial * (1.0F / initialNorm);
    const float dot = (initial.Get(0, 0) * normedNow.Get(0, 0)) +
                      (initial.Get(1, 0) * normedNow.Get(1, 0)) +
                      (initial.Get(2, 0) * normedNow.Get(2, 0));
    const float offStart = std::acos(dot);
    return {orientation, offStart > (30.0F * 3.14159F / 180.0F)};
}

double MaxOrientationDiff(const Matrix3& lhs, const float* rhs)
{
    double maxDiff = 0.0;
    for (int i = 0; i < 9; ++i)
    {
        const int row = i / 3;
        const int col = i % 3;
        maxDiff = std::max(maxDiff, std::abs(static_cast<double>(lhs.Get(row, col)) - static_cast<double>(rhs[i])));
    }
    return maxDiff;
}

struct Stats {
    double minNs;
    double meanNs;
};

template <typename Fn>
Stats RunBenchmark(const char* name, Fn&& fn, size_t iterations, size_t rounds)
{
    std::vector<double> samples;
    samples.reserve(rounds);
    for (size_t round = 0; round < rounds; ++round)
    {
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

void RunBehaviorCase(const char* name, float gx, float gy, float gz, uint32_t dtMs, size_t steps)
{
    Matrix3 originalOrientation = Matrix3::Identity();
    bool oldEverOut = false;
    int firstOldOutStep = -1;
    int firstNewOutStep = -1;
    double maxOrientationDiff = 0.0;

    NModel::ResetGyroStateForBenchmark();
    uint32_t timestampMs = 0;
    for (size_t i = 0; i < steps; ++i)
    {
        const auto oldResult = FeedGyroOriginal(originalOrientation, gx, gy, gz, static_cast<float>(dtMs) / 1000.0F);
        originalOrientation = oldResult.orientation;
        if ((firstOldOutStep < 0) && oldResult.outOfBounds)
        {
            firstOldOutStep = static_cast<int>(i);
        }
        oldEverOut |= oldResult.outOfBounds;

        timestampMs += dtMs;
        NModel::FeedGyro(timestampMs, {.X = gx, .Y = gy, .Z = gz});
        float orientationMatrix[9] = {};
        NModel::FillPacketWithOrientationMatrix(orientationMatrix);
        maxOrientationDiff = std::max(maxOrientationDiff, MaxOrientationDiff(originalOrientation, orientationMatrix));
        if ((firstNewOutStep < 0) && NModel::EverWentOutOfBounds())
        {
            firstNewOutStep = static_cast<int>(i);
        }
    }

    std::printf("%s\n", name);
    std::printf("  max orientation diff: %.6f\n", maxOrientationDiff);
    std::printf("  first old out step:   %d\n", firstOldOutStep);
    std::printf("  first new out step:   %d\n", firstNewOutStep);
    DoNotOptimize(oldEverOut);
}

} // namespace

namespace NModel {
void RotateRocketVectorToIMUVector(const NTypes::AccelerometerData& xyz, NTypes::AccelerometerData& out)
{
    const auto rotated = RotateRocketToImu({xyz.X, xyz.Y, xyz.Z});
    out.X = rotated[0];
    out.Y = rotated[1];
    out.Z = rotated[2];
}
} // namespace NModel

int main()
{
    constexpr size_t kIterations = 1'000'000;
    constexpr size_t kRounds = 8;

    volatile float sink = 0.0F;
    volatile bool sinkBool = false;

    for (size_t i = 0; i < 10000; ++i)
    {
        const float gx = 0.02F + static_cast<float>(i % 11) * 0.004F;
        const float gy = -0.03F + static_cast<float>(i % 7) * 0.005F;
        const float gz = 0.01F + static_cast<float>(i % 13) * 0.003F;
        sink += ExpGyroOriginal(gx, gy, gz, 0.01F).Get(0, 0);
        sink += NModel::expGyro(gx, gy, gz, 0.01F).Get(0, 0);
        sink += AltitudeOriginal(85.0F + static_cast<float>(i % 50) * 0.1F);
        sink += NModel::AltitudeMetersFromPressureKPa(85.0F + static_cast<float>(i % 50) * 0.1F);
    }

    std::puts("Airbraker math benchmark");
    std::printf("iterations=%zu rounds=%zu\n\n", kIterations, kRounds);

    const auto expOriginal = RunBenchmark("expGyro original", [&](size_t iterations) {
        for (size_t i = 0; i < iterations; ++i)
        {
            const float gx = 0.02F + static_cast<float>(i % 11) * 0.004F;
            const float gy = -0.03F + static_cast<float>(i % 7) * 0.005F;
            const float gz = 0.01F + static_cast<float>(i % 13) * 0.003F;
            sink += ExpGyroOriginal(gx, gy, gz, 0.01F).Get(0, 0);
        }
        DoNotOptimize(sink);
    }, kIterations, kRounds);

    const auto expNew = RunBenchmark("expGyro new", [&](size_t iterations) {
        for (size_t i = 0; i < iterations; ++i)
        {
            const float gx = 0.02F + static_cast<float>(i % 11) * 0.004F;
            const float gy = -0.03F + static_cast<float>(i % 7) * 0.005F;
            const float gz = 0.01F + static_cast<float>(i % 13) * 0.003F;
            sink += NModel::expGyro(gx, gy, gz, 0.01F).Get(0, 0);
        }
        DoNotOptimize(sink);
    }, kIterations, kRounds);

    const auto altOriginal = RunBenchmark("altitude original", [&](size_t iterations) {
        for (size_t i = 0; i < iterations; ++i)
        {
            sink += AltitudeOriginal(80.0F + static_cast<float>(i % 1000) * 0.02F);
        }
        DoNotOptimize(sink);
    }, kIterations, kRounds);

    const auto altNew = RunBenchmark("altitude new", [&](size_t iterations) {
        for (size_t i = 0; i < iterations; ++i)
        {
            sink += NModel::AltitudeMetersFromPressureKPa(80.0F + static_cast<float>(i % 1000) * 0.02F);
        }
        DoNotOptimize(sink);
    }, kIterations, kRounds);

    const auto feedOriginal = RunBenchmark("feedGyro original", [&](size_t iterations) {
        Matrix3 orientation = Matrix3::Identity();
        bool everOut = false;
        for (size_t i = 0; i < iterations; ++i)
        {
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
    }, kIterations, kRounds);

    const auto feedNew = RunBenchmark("feedGyro new", [&](size_t iterations) {
        NModel::ResetGyroStateForBenchmark();
        bool everOut = false;
        uint32_t timestampMs = 0;
        float orientationMatrix[9] = {};
        for (size_t i = 0; i < iterations; ++i)
        {
            const float gx = 0.02F + static_cast<float>(i % 11) * 0.004F;
            const float gy = -0.03F + static_cast<float>(i % 7) * 0.005F;
            const float gz = 0.01F + static_cast<float>(i % 13) * 0.003F;
            timestampMs += 10;
            NModel::FeedGyro(timestampMs, {.X = gx, .Y = gy, .Z = gz});
            everOut |= NModel::EverWentOutOfBounds();
        }
        NModel::FillPacketWithOrientationMatrix(orientationMatrix);
        sink += orientationMatrix[0];
        sinkBool ^= everOut;
        DoNotOptimize(sink);
        DoNotOptimize(sinkBool);
    }, kIterations, kRounds);

    std::puts("\nSpeedups based on best run");
    std::printf("expGyro:        %.2fx\n", expOriginal.minNs / expNew.minNs);
    std::printf("altitude:       %.2fx\n", altOriginal.minNs / altNew.minNs);
    std::printf("feedGyro step:  %.2fx\n", feedOriginal.minNs / feedNew.minNs);

    std::puts("\nBehavior checks");
    RunBehaviorCase("zero gyro", 0.0F, 0.0F, 0.0F, 10, 800);
    RunBehaviorCase("constant pitch", 0.10F, 0.0F, 0.0F, 10, 800);
    const auto rocketAxisInImu = RotateRocketToImu({0.0F, 0.0F, 1.0F});
    RunBehaviorCase("spin about rocket axis", rocketAxisInImu[0] * 0.10F, rocketAxisInImu[1] * 0.10F,
                    rocketAxisInImu[2] * 0.10F, 10, 800);

    std::printf("\nignore sinks: %f %d\n", sink, static_cast<int>(sinkBool));
    return 0;
}
