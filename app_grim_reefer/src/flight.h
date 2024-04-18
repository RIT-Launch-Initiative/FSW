#pragma once
#include <stdint.h>

enum flight_event {
  flight_event_boost,
  flight_event_noseover,
  flight_event_main_deploy,
  flight_event_main_landed,
  flight_event_main_shutoff,
};
