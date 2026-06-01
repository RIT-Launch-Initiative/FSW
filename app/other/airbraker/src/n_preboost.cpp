#include "n_preboost.hpp"

#include "math/matrix.hpp"
#include "n_model.hpp"

#include <f_core/utils/linear_fit.hpp>
#include <zsl/orientation/quaternions.h>

namespace NTypes

{
AccelerometerData operator+(const AccelerometerData &lhs, const AccelerometerData &rhs) {
    return {
        .X = lhs.X + rhs.X,
        .Y = lhs.Y + rhs.Y,
        .Z = lhs.Z + rhs.Z,
    };
}

AccelerometerData operator-(const AccelerometerData &lhs, const AccelerometerData &rhs) {
    return {
        .X = lhs.X - rhs.X,
        .Y = lhs.Y - rhs.Y,
        .Z = lhs.Z - rhs.Z,
    };
}
AccelerometerData operator/(const AccelerometerData &lhs, float rhs) {
    return {
        .X = lhs.X / rhs,
        .Y = lhs.Y / rhs,
        .Z = lhs.Z / rhs,
    };
}

} // namespace NTypes

namespace NPreBoost {

// holder for NTypes::GyroscopeData that adds +,-,/ operators
struct GyroAxes {
    NTypes::GyroscopeData internal;

    GyroAxes operator+(const GyroAxes &rhs) const {
        return {.internal = {
                    .X = this->internal.X + rhs.internal.X,
                    .Y = this->internal.Y + rhs.internal.Y,
                    .Z = this->internal.Z + rhs.internal.Z,
                }};
    }
    GyroAxes operator-(const GyroAxes &rhs) const {
        return {.internal = {
                    .X = this->internal.X - rhs.internal.X,
                    .Y = this->internal.Y - rhs.internal.Y,
                    .Z = this->internal.Z - rhs.internal.Z,
                }};
    }
    GyroAxes operator/(float rhs) const {
        return {.internal = {
                    .X = this->internal.X / rhs,
                    .Y = this->internal.Y / rhs,
                    .Z = this->internal.Z / rhs,
                }};
    }
};

Packet zeroPacket = Packet{0};
GyroAxes zeroAxes = {0};

CMovingAverage<NTypes::AccelerometerData, NUM_SAMPLES_FOR_GYRO_BIAS> rodAccelAverager{{0, 0, 1}};
CMovingAverage<GyroAxes, NUM_SAMPLES_FOR_GYRO_BIAS> gyroBiasAverager{zeroAxes};
CCircularBuffer<Packet, NUM_STORED_PREBOOST_PACKETS> preboostPackets{zeroPacket};
float groundLevelPressure = 0;
float groundLevelASLMeters = 0; // altitude from pressure reading at our most current idea of before launch

zsl_quat onRailOrientationEstimate{1, 0, 0, 0};
zsl_quat GetOnRodOrientation() { return onRailOrientationEstimate; }

void RocketImuReadingToRailOrientation(const NTypes::AccelerometerData &accelRocketSpaceData) {
    Matrix<3, 1> accelRocketSpace{{accelRocketSpaceData.X, accelRocketSpaceData.Y, accelRocketSpaceData.Z}};
    float normRocketSpace = std::sqrt(accelRocketSpace.Get(0, 0) * accelRocketSpace.Get(0, 0) +
                                      accelRocketSpace.Get(1, 0) * accelRocketSpace.Get(1, 0) +
                                      accelRocketSpace.Get(2, 0) * accelRocketSpace.Get(2, 0));

    auto cross = [](const Matrix<3, 1> &a, const Matrix<3, 1> &b) {
        float x = (a.Get(1, 0) * b.Get(2, 0)) - (a.Get(2, 0) * b.Get(1, 0));
        float y = -((a.Get(0, 0) * b.Get(2, 0)) - (a.Get(2, 0) * b.Get(0, 0)));
        float z = (a.Get(0, 0) * b.Get(1, 0)) - (a.Get(1, 0) * b.Get(0, 0));
        return Matrix<3, 1>{{x, y, z}};
    };

    Matrix<3, 1> gravity{{0, 0, 1}};
    Matrix<3, 1> vec = accelRocketSpace * (1.F / normRocketSpace); // normalized rocket space IMU
    Matrix<3, 1> u = cross(vec, gravity);
    float theta = std::acos(dot(vec, gravity));
    float w = std::cos(theta / 2);
    float x = u.Get(0, 0) * std::sin(theta / 2);
    float y = u.Get(1, 0) * std::sin(theta / 2);
    float z = u.Get(2, 0) * std::sin(theta / 2);

    onRailOrientationEstimate = zsl_quat{.r = w, .i = x, .j = y, .k = z};
}

void SubmitPreBoostPacket(const Packet &packet) {
    // newest sample for gyro bias
    preboostPackets.AddSample(packet);
    Packet *newestSampleForGyroBias = &preboostPackets.OldestSample();
    gyroBiasAverager.Feed({newestSampleForGyroBias->gyro});
    rodAccelAverager.Feed(newestSampleForGyroBias->accelRaw);

    NTypes::AccelerometerData rocket_space_imu{};
    RotateIMUVectorToRocketVector(rodAccelAverager.Avg(), rocket_space_imu);
    RocketImuReadingToRailOrientation(rocket_space_imu);

    // Grab ground level altitude from most recent if we're before the circular buffer is initialized
    if (preboostPackets.OldestSample().pressureRaw == 0) {
        groundLevelPressure = packet.pressureRaw;
    } else {
        groundLevelPressure = preboostPackets.OldestSample().pressureRaw;
    }
    groundLevelASLMeters = NModel::AltitudeMetersFromPressureKPa(groundLevelPressure);
}

NTypes::GyroscopeData GetGyroBias() { return gyroBiasAverager.Avg().internal; }

float GetGroundLevelASL() { return groundLevelASLMeters; }
float GetGroundLevelPressure() { return groundLevelPressure; }

void GetPreBoostPacket(size_t index, Packet &packetOut) { packetOut = preboostPackets[index]; }
Packet *GetPreBoostPacketPtr(size_t index) { return &preboostPackets[index]; }

}; // namespace NPreBoost