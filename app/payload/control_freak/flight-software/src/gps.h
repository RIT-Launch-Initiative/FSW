#ifndef FLIGHT_GPS_H
#define FLIGHT_GPS_H
#include "f_core/radio/protocols/horus/horus.h"

#include <stdint.h>

// Retrieve GPS part of packed data for slow packet, non-gps bits are left as 0
int get_packed_gps(uint64_t &packed_data, int16_t &lat_frac, int16_t &long_frac);

int fill_packet(struct horus_packet_v2 *packet);

uint32_t micros_till_timeslot_opens();
float get_skew_smart();

/**
 * If a momentary powercycle occurs, the GPS will sometimes stop reporting valid packets and timepulses
 * call this to try to reset it if necessary
 * The inside of this function 
 */
int reset_gps_if_acting_funny();

#endif