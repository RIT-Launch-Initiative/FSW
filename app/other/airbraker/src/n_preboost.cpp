#include "n_preboost.hpp"

#include "n_model.hpp"

#include <f_core/utils/linear_fit.hpp>
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

CMovingAverage<GyroAxes, NUM_SAMPLES_FOR_GYRO_BIAS> gyroBiasAverager{zeroAxes};
CCircularBuffer<Packet, NUM_STORED_PREBOOST_PACKETS> preboostPackets{zeroPacket};
float groundLevelPressure = 0;
float groundLevelASLMeters = 0; // altitude from pressure reading at our most current idea of before launch

void SubmitPreBoostPacket(const Packet &packet) {
    // newest sample for gyro bias
    preboostPackets.AddSample(packet);
    Packet *newestSampleForGyroBias = &preboostPackets.OldestSample();
    gyroBiasAverager.Feed({newestSampleForGyroBias->gyro});
    // Grab ground level altitude from most recent if we're before the circular buffer is initialized
    if (preboostPackets.OldestSample().pressureRaw == 0) {
        groundLevelPressure = packet.pressureRaw;
    } else {
        groundLevelPressure = preboostPackets.OldestSample().pressureRaw;
    }
}

NTypes::GyroscopeData GetGyroBias() { return gyroBiasAverager.Avg().internal; }

float GetGroundLevelASL() { return groundLevelASLMeters; }
float GetGroundLevelPressure() { return groundLevelPressure; }

void GetPreBoostPacket(size_t index, Packet &packetOut) { packetOut = preboostPackets[index]; }
Packet *GetPreBoostPacketPtr(size_t index) { return &preboostPackets[index]; }

}; // namespace NPreBoost