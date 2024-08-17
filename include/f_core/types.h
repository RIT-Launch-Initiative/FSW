#ifndef N_SENSOR_DATA_TYPES_H
#define N_SENSOR_DATA_TYPES_H

#include <stdint.h>

#define FLOAT_ERROR_VALUE FLT_MIN

/****** Primitive Data Types ******/
// TODO: See if we can support __fp16 in GSW first
// typedef __fp16 float16_t;

namespace NTypes
{
    namespace NSensor
    {
        /****** Telemetry Data Types ******/
        typedef struct __attribute__((packed))
        {
            float X;
            float Y;
            float Z;
        } AccelerometerData;

        typedef struct __attribute__((packed))
        {
            float Pressure;
            float Temperature;
        } BarometerData;

        typedef struct __attribute__((packed))
        {
            float X;
            float Y;
            float Z;
        } GyroscopeData;

        typedef struct __attribute__((packed))
        {
            float X;
            float Y;
            float Z;
        } MagnetometerData;

        typedef struct __attribute__((packed))
        {
            float Current;
            float Voltage;
            float Power;
        } ShuntData;

        typedef float TemperatureData;
    }

    typedef struct __attribute__((packed))
    {
        double Latitude;
        double Longitude;
        float Altitude;
    } GnssCoordinateData;

    typedef struct __attribute__((packed))
    {
        uint64_t CpuTimeMs;
        uint64_t GnssTime;
        gnss_data_t GnssData;
    } GnssTimeSynchronizationdata;

}

#endif
