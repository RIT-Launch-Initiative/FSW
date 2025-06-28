#include <f_core/math/filters/c_madgwick.h>
#include <f_core/device/sensor/c_accelerometer.h>
#include <f_core/device/sensor/c_gyroscope.h>
#include <f_core/device/sensor/c_magnetometer.h>

int main() {
    CMadgwick madgwickFilter(100.0);
    CAccelerometer accelerometer(*DEVICE_DT_GET(DT_ALIAS(imu)));
    CGyroscope gyroscope(*DEVICE_DT_GET(DT_ALIAS(gyroscope)));
    CMagnetometer magnetometer(*DEVICE_DT_GET(DT_ALIAS(magnetometer)));

    zsl_mtx accelInitData[100]{0};
    zsl_mtx gyroInitData[100]{0};
    zsl_mtx magnInitData[100]{0};

    for (int i = 0; i < 100; i++) {
        accelerometer.UpdateSensorValue();
        gyroscope.UpdateSensorValue();
        magnetometer.UpdateSensorValue();

        accelInitData[i].data[0] = accelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_X);
        accelInitData[i].data[1] = accelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_Y);
        accelInitData[i].data[2] = accelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_Z);

        gyroInitData[i].data[0] = gyroscope.GetSensorValueFloat(SENSOR_CHAN_GYRO_X);
        gyroInitData[i].data[1] = gyroscope.GetSensorValueFloat(SENSOR_CHAN_GYRO_Y);
        gyroInitData[i].data[2] = gyroscope.GetSensorValueFloat(SENSOR_CHAN_GYRO_Z);

        magnInitData[i].data[0] = magnetometer.GetSensorValueFloat(SENSOR_CHAN_MAGN_X);
        magnInitData[i].data[1] = magnetometer.GetSensorValueFloat(SENSOR_CHAN_MAGN_Y);
        magnInitData[i].data[2] = magnetometer.GetSensorValueFloat(SENSOR_CHAN_MAGN_Z);

        k_msleep(10);
    }

    int ret = madgwickFilter.CalibrateBetaTerm(accelInitData, gyroInitData, magnInitData, nullptr);
    if (ret < 0) {
        return ret;
    }

    zsl_real_t accelX = 0.0;
    zsl_real_t accelY = 0.0;
    zsl_real_t accelZ = 0.0;

    zsl_real_t gyroX = 0.0;
    zsl_real_t gyroY = 0.0;
    zsl_real_t gyroZ = 0.0;

    zsl_real_t magnX = 0.0;
    zsl_real_t magnY = 0.0;
    zsl_real_t magnZ = 0.0;

    while (true) {


    }








    return 0;
}
