#ifndef FREAK_GORBFS_H
#define FREAK_GORBFS_H
#include <stdbool.h>
#include <stdint.h>
struct acc_data {
    float ax; // m/s^2
    float ay; // m/s^2
    float az; // m/s^2
};
struct gyro_data {
    float gx; // rad/s
    float gy; // rad/s
    float gz; // rad/s
};

#define IMU_SAMPLES_PER_PACKET 10
struct SuperFastPacket {
    int64_t timestamp;
    float temp;
    float pressure;
    struct gyro_data gdat[IMU_SAMPLES_PER_PACKET];
    struct acc_data adat[IMU_SAMPLES_PER_PACKET];
};

// https://support.garmin.com/en-US/?faq=hRMBoCTy5a7HqVkxukhHd8
// 1 degree = 111km
// 1/65536 degree = 1.69m

//   31.94°  lat
// -102.19°  long
//  874   m  alt (ASL)
//  800   m/s  max speed
// 86400000 in a day

// 14 bits alt (1 ft / LSB) (0-16384 in case we overshoot or fall into hell)
// 10 bits speed (1 m/s / LSB) (0-1024m/s (OR says max 800))
// 2 bits fix status (0 None, 1 GNSS Fix, 2 DGNSS Fix, 3 Estimated Fix)
// 4 bits satelites (max 16 in sight at once over est. http://csno-tarc.cn/en/gps/number)

// 3 bits fix quality (see zephyr enum)

// 27 bits millis since start of day

// 3 bits flight state (0: notready, 1 waiting, 2 boost, 3 coast, 4 ground, 5 flipping, 6 transmitting, 7 error)
// 1 bit for fun? parity?

typedef uint64_t packed_data;

struct SlowInf {
    int64_t timestamp;
    packed_data gps_info;

    uint16_t lat_frac;  // 1/65536 degree
    uint16_t long_frac; // 1/65536 degree

    int16_t orientation[3];

    int16_t battery_voltage;
    uint16_t instantaneous_current;

    uint8_t temp_c;
    uint8_t flip_status; // flip status (over currented, all good)
};

typedef struct SlowInf SuperSlowPacket[8];

#endif