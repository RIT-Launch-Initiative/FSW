#include "sensors.h"
#include "orchestrator.h"

#ifdef BOARD_GRIM_REEFER
#error havent implemented sensor reading yet, sue me
#else

// #error i also haven't implemented fake sensor reading yet
all_inas_data_t read_inas() {
  ina260_data_t battery = {
      .millivolts = 3300,
      .milliamps = 1000,
  };
  ina260_data_t load_cell = {
      .millivolts = 3300,
      .milliamps = 1000,
  };
  ina260_data_t board = {
      .millivolts = 3300,
      .milliamps = 1000,
  };

  all_inas_data_t all = {
      battery,
      load_cell,
      board,
  };
  return all;
}

#endif