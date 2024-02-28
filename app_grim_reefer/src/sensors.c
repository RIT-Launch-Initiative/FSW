#include "sensors.h"
#include "orchestrator.h"

#ifdef BOARD_GRIM_REEFER
#error havent implemented sensor reading yet, sue me
#else
int read_slow(struct slow_data *dat) {
  dat->humidty = 0;
  dat->temperature = 0;
  dat->grim_voltage = 0;
  dat->grim_current = 0;

  dat->load_cell_voltage = 0;
  dat->load_cell_current = 0;

  dat->cam_voltage = 0;
  dat->cam_current = 0;
  return 0;
}
int read_fast(struct fast_data *dat) {
  dat->accel_x = 0x6c6c6548;
  dat->accel_y = 0x7665596f;
  dat->accel_z = 0x6c6c6548;

  dat->gyro_x = 0x7665596f;
  dat->gyro_y = 0x6c6c6548;
  dat->gyro_z = 0x7665596f;

  dat->altimeter = 0x333c333c;

  return 0;
}
int read_adc(struct adc_data *dat) {
  dat->adc_value = 0;
  return 0;
}
#endif