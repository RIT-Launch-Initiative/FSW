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
struct SuperFastPacket {
    int64_t timestamp;
    float temp;
    float pressure;
    struct gyro_data gdat[10];
    struct acc_data adat[10];
};

void lock_boostdata();
void unlock_boostdata();

bool is_boostdata_locked();

#endif