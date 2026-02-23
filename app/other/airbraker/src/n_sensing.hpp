#include "common.hpp"
namespace NSensing {
int init_sensors();
/**
 * @param[out] tempC temperature in celcius from chosen barometer
 * @param[out] pressure_Kpa pressure in kilopascals from chosen barometer
 * @param[out] accel_ms2 vertical acceleration from chosen IMU (TODO)
 * @param[out] gyro_dps 3axis gyroscope reading from chosen IMU
 */
int MeasureSensors(float &temp_C, float &pressure_kPa, float &accel_ms2, NTypes::GyroscopeData &gyro_dps);

}; // namespace NSensing