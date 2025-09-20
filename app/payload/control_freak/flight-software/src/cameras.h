#pragma once
int camera_thread_entry(void *v_fc, void *, void *);

void both_cams_on();
void both_cams_off();

int update_battery_voltage(const float &bat_voltage);