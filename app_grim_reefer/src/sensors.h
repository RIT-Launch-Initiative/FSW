#include <stdint.h>

uint32_t useconds_since_launch();

// sorta made up types
typedef struct {
  int32_t x, y, z;
  int32_t rx, ry, rz;
} imu_data_t;
typedef struct {
  int32_t ft_above_sealevel;
} altim_data_t;

typedef struct {
  int32_t millivolts;
} adc_data_t;

typedef struct {
  int32_t temp;
  int32_t pressure;
  int32_t humidity;
} bme280_data_t;

typedef struct {
  int32_t millivolts;
  int32_t milliamps;
} ina260_data_t;

typedef struct {
  ina260_data_t battery;
  ina260_data_t load_cell;
  ina260_data_t board_power;
} all_inas_data_t;

all_inas_data_t read_inas();