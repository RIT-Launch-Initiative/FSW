#pragma once
#include <stdint.h>
/*
Responsible for changing the state of the program

Ground    -> data dumping mode
Launch Detection
Noseover detection / timer
Main detection / timer
Ground detection / timer
*/

#define EVENT_HAPPENED (1)

extern struct k_event launch_detected;
extern struct k_event noseover_detected;
extern struct k_event main_detected;
extern struct k_event flight_over;

uint32_t useconds_since_launch();

int orchestrate();