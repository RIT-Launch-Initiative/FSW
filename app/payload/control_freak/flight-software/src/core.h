#include "f_core/radio/protocols/horus/horus.h"

#include <zephyr/kernel.h>

horus_packet_v2 get_telemetry();
bool has_fix();

// thing to get clock drift
float estimate_clock_skew();

k_ticks_t ticks_till_next_timeslot();