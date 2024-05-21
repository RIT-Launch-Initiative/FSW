#ifndef REEFER_INLCUDE_FLIGHT_H
#define REEFER_INLCUDE_FLIGHT_H

#include <stdint.h>

enum flight_event {
    flight_event_boost,
    flight_event_noseover,
    flight_event_main_deploy,
    flight_event_main_landed,
    flight_event_main_shutoff,
};

#endif