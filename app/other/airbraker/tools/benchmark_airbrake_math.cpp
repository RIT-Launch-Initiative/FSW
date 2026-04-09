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

constexpr std::array<float, 7> kAtmosphere = {
    1.2345e2F, -1.876e-2F, 3.4e-6F, -5.1e-10F, 7.2e-14F, -3.3e-18F, 6.8e-23F,
};

constexpr std::array<std::array<float, 3>, 3> kImuToRocket = {{
    {0.9961947F, -0.0121320F, 0.0864101F},
    {0.0151344F, 0.9993007F, -0.0336292F},
    {-0.0858317F, 0.0349465F, 0.9956955F},
}};

template <typename T>
inline void DoNotOptimize(T const& value)
{
    asm volatile("" : : "g"(value) : "memory");
}

float ClampUnit(float v)
{
    return std::max(-1.0F, std::min(1.0F, v));
}

float Dot3(const Vector3& a, const Vector3& b)
{
    return (a.Get(0, 0) * b.Get(0, 0)) + (a.Get(1, 0) * b.Get(1, 0)) + (a.Get(2, 0) * b.Get(2, 0));
}

std::array<float, 3> RotateImuToRocket(const std::array<float, 3>& v)
{
    std::array<float, 3> out{};
    for (int r = 0; r < 3; ++r)
    {
        out[r] = (kImuToRocket[r][0] * v[0]) + (kImuToRocket[r][1] * v[1]) + (kImuToRocket[r][2] * v[2]);
    }
    return out;
}

std::array<float, 3> RotateRocketToImu(const std::array<float, 3>& v)
{
    std::array<float, 3> out{};
    for (int r = 0; r < 3; ++r)
    {
        out[r] = (kImuToRocket[0][r] * v[0]) + (kImuToRocket[1][r] * v[1]) + (kImuToRocket[2][r] * v[2]);
    }
    return out;
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

Matrix3 ExpGyroNew(float w1, float w2, float w3, float t)
{
    const float wx = w1 * t;
    const float wy = w2 * t;
    const float wz = w3 * t;

    const float wx2 = wx * wx;
    const float wy2 = wy * wy;
    const float wz2 = wz * wz;
    const float wxwy = wx * wy;
    const float wxwz = wx * wz;
    const float wywz = wy * wz;

    const float thetaSq = wx2 + wy2 + wz2;
    const float theta = std::sqrt(thetaSq);
    const float s = (theta == 0.0F) ? 1.0F : (std::sin(theta) / theta);
    const float c = (theta == 0.0F) ? 0.0F : ((1.0F - std::cos(theta)) / thetaSq);

    return Matrix3{{
        1.0F - c * (wy2 + wz2), c * wxwy - s * wz, c * wxwz + s * wy,
        c * wxwy + s * wz, 1.0F - c * (wx2 + wz2), c * wywz - s * wx,
        c * wxwz - s * wy, c * wywz + s * wx, 1.0F - c * (wx2 + wy2),
    }};
}

float AltitudeOriginal(float kPa)
{
    const float x = kPa * 1000.0F;
    float sum = 0.0F;
    float xn = x;
    for (std::size_t i = 1; i < kAtmosphere.size(); ++i)
    {
        sum += kAtmosphere[i] * xn;
        xn *= x;
    }
    return sum + kAtmosphere[0];
}

float AltitudeNew(float kPa)
{
    const float x = kPa * 1000.0F;
    float sum = kAtmosphere[kAtmosphere.size() - 1];
    for (std::size_t i = kAtmosphere.size() - 1; i-- > 0;)
    {
        sum = (sum * x) + kAtmosphere[i];
    }
    return sum;
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
    const auto rotated = RotateImuToRocket({now.Get(0, 0), now.Get(1, 0), now.Get(2, 0)});

    (void)rotated;

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

GyroStepResult FeedGyroNew(Matrix3 orientation, float gx, float gy, float gz, float dt)
{
    constexpr float kCosMaxTilt = 0.8660254037844386F;
    static const Vector3 initial = [] {
        const auto zInImu = RotateRocketToImu({0.0F, 0.0F, 1.0F});
        return Vector3{{zInImu[0], zInImu[1], zInImu[2]}};
    }();

    const Matrix3 eAT = ExpGyroNew(gx, gy, gz, dt);
    orientation = orientation * eAT;

    const Vector3 now = orientation * initial;
    const float nowNorm = std::sqrt(Dot3(now, now));
    const float cosOffStart = (nowNorm == 0.0F) ? 1.0F : ClampUnit(Dot3(initial, now) / nowNorm);
    return {orientation, cosOffStart < kCosMaxTilt};
}

struct Stats {
    double minNs;
    double meanNs;
};

template <typename Fn>
Stats RunBenchmark(const char* name, Fn&& fn, std::size_t iterations, std::size_t rounds)
{
    std::vector<double> samples;
    samples.reserve(rounds);
    for (std::size_t round = 0; round < rounds; ++round)
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

}  // namespace

int main()
{
    constexpr std::size_t kIterations = 1'000'000;
    constexpr std::size_t kRounds = 8;

    volatile float sink = 0.0F;
    volatile bool sinkBool = false;

    for (std::size_t i = 0; i < 10000; ++i)
    {
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

    const auto expOriginal = RunBenchmark("expGyro original", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i)
        {
            const float gx = 0.02F + static_cast<float>(i % 11) * 0.004F;
            const float gy = -0.03F + static_cast<float>(i % 7) * 0.005F;
            const float gz = 0.01F + static_cast<float>(i % 13) * 0.003F;
            sink += ExpGyroOriginal(gx, gy, gz, 0.01F).Get(0, 0);
        }
        DoNotOptimize(sink);
    }, kIterations, kRounds);

    const auto expNew = RunBenchmark("expGyro new", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i)
        {
            const float gx = 0.02F + static_cast<float>(i % 11) * 0.004F;
            const float gy = -0.03F + static_cast<float>(i % 7) * 0.005F;
            const float gz = 0.01F + static_cast<float>(i % 13) * 0.003F;
            sink += ExpGyroNew(gx, gy, gz, 0.01F).Get(0, 0);
        }
        DoNotOptimize(sink);
    }, kIterations, kRounds);

    const auto altOriginal = RunBenchmark("altitude original", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i)
        {
            sink += AltitudeOriginal(80.0F + static_cast<float>(i % 1000) * 0.02F);
        }
        DoNotOptimize(sink);
    }, kIterations, kRounds);

    const auto altNew = RunBenchmark("altitude new", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i)
        {
            sink += AltitudeNew(80.0F + static_cast<float>(i % 1000) * 0.02F);
        }
        DoNotOptimize(sink);
    }, kIterations, kRounds);

    const auto feedOriginal = RunBenchmark("feedGyro original", [&](std::size_t iterations) {
        Matrix3 orientation = Matrix3::Identity();
        bool everOut = false;
        for (std::size_t i = 0; i < iterations; ++i)
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

    const auto feedNew = RunBenchmark("feedGyro new", [&](std::size_t iterations) {
        Matrix3 orientation = Matrix3::Identity();
        bool everOut = false;
        for (std::size_t i = 0; i < iterations; ++i)
        {
            const float gx = 0.02F + static_cast<float>(i % 11) * 0.004F;
            const float gy = -0.03F + static_cast<float>(i % 7) * 0.005F;
            const float gz = 0.01F + static_cast<float>(i % 13) * 0.003F;
            const auto result = FeedGyroNew(orientation, gx, gy, gz, 0.01F);
            orientation = result.orientation;
            everOut |= result.outOfBounds;
        }
        sink += orientation.Get(0, 0);
        sinkBool ^= everOut;
        DoNotOptimize(sink);
        DoNotOptimize(sinkBool);
    }, kIterations, kRounds);

    std::puts("\nSpeedups based on best run");
    std::printf("expGyro:        %.2fx\n", expOriginal.minNs / expNew.minNs);
    std::printf("altitude:       %.2fx\n", altOriginal.minNs / altNew.minNs);
    std::printf("feedGyro step:  %.2fx\n", feedOriginal.minNs / feedNew.minNs);
    std::printf("\nignore sinks: %f %d\n", sink, static_cast<int>(sinkBool));
    return 0;
}
