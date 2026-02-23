#include "common.hpp"
namespace NSensing {
int InitSensors();
/**
 * @param[out] tempC temperature in celcius from chosen barometer
 * @param[out] pressure_Kpa pressure in kilopascals from chosen barometer
 * @param[out] accelMs2 vertical acceleration from chosen IMU (TODO)
 * @param[out] gyroDps 3axis gyroscope reading from chosen IMU
 */
int MeasureSensors(float &tempC, float &pressureKPa, float &accelMs2, NTypes::GyroscopeData &gyroDps);

}; // namespace NSensing