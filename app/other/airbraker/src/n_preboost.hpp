#include "common.hpp"
#include "n_boost.hpp"
namespace NPreBoost {

/**
 * Submit a preboost packet to temporary storage
 * @param packet the packet that will enter the circular buffer. 
 * If boost does not happen before "NUM_STORED_PREBOOST_PACKETS" more packets are submitted,
 * this packet will be overwritten by a newer one
 * After boost is detected, the packets will not change and can be written to persistent storage
 * Note: this is also the mechanism through which the gyro bias estimator is fed
 */
void SubmitPreBoostPacket(const Packet &packet);

/** 
 * Return the accumulated gyroscope bias to calibrate for gyro integration
*/
NTypes::GyroscopeData GetGyroBias();

/**
 * Return the altitude we found at ground level before launch
 * @return meters over sea level
 */
float GetGroundLevelASL();

/**
 * Return the pressure in kPa we found at ground level before launch
 * @return pressure value in kPa
 */
float GetGroundLevelPressure();

/**
 * Get PreBoostPacket for later storage or just looking
 * @param[in] index of packet to get
 * @param[out] packetOut out parameter of packet that will be filled in
 */
void GetPreBoostPacket(size_t index, Packet &packetOut);

/**
 * Get a pointer to a preboost packet
 * @param index index of preboost packet (0 is oldest)
 * @return pointer into circular buffer. 
 * Data at pointer will be constantly overwritten while new data is fed in. 
 * Don't expect the data at the end of this pointer to hold constant data until you stop giving preboost packets
 * Don't do anything thread unsafe with this
 */
Packet *GetPreBoostPacketPtr(size_t index);

} // namespace NPreBoost