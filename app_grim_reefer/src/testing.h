#pragma once
#include <zephyr/drivers/adc.h>

void adc_printout(const struct adc_dt_spec *channel);
void print_statvfs(char *fname);