/// THIS CODE IS GENERATED BY AN AUTOCODER (DO NOT EDIT)
// Last Generated: 2025-03-25 20:02:12.257412
// Reference Files: 
// - /home/launchlinux/FSW/data/autocoder_inputs/types.yaml

#ifndef _AUTOCODER_TYPES_H_
#define _AUTOCODER_TYPES_H_

#include <stdint.h>

namespace NTypes { 
    
    // Accelerometer telemetry data.
    
    typedef struct __attribute__((packed)) {
        float X; 
        float Y; 
        float Z; 
    } AccelerometerData;
    
    
    typedef struct __attribute__((packed)) {
        uint32_t timestamp;
        AccelerometerData data;
    } TimestampedAccelerometerData;
    
    
    
    // Barometer telemetry data.
    
    typedef struct __attribute__((packed)) {
        float Pressure; 
        float Temperature; 
    } BarometerData;
    
    
    
    // Gyroscope telemetry data.
    
    typedef struct __attribute__((packed)) {
        float X; 
        float Y; 
        float Z; 
    } GyroscopeData;
    
    
    
    // Magnetometer telemetry data.
    
    typedef struct __attribute__((packed)) {
        float X; 
        float Y; 
        float Z; 
    } MagnetometerData;
    
    
    
    // Shunt telemetry data.
    
    typedef struct __attribute__((packed)) {
        float Voltage; 
        float Current; 
        float Power; 
    } ShuntData;
    
    
    
    // Single temperature reading.
    
    typedef struct __attribute__((packed)) {
        float Temperature; 
    } TemperatureData;
    
    
    
    // LoRa signal data.
    
    typedef struct __attribute__((packed)) {
        float RSSI; 
        float SNR; 
    } LoRaSignalData;
    
    
    
    // GNSS coordinate data.
    
    typedef struct __attribute__((packed)) {
        double Latitude; 
        double Longitude; 
        float Altitude; 
    } GnssCoordinateData;
    
    
    }

#endif // _AUTOCODER_TYPES_H_