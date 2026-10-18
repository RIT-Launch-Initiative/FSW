#pragma once
#include <stdint.h>

/* 
 *  Call once to initialize the servo subsystem
 *  Intended for use in SYS_INIT
 */
extern "C" int servo_init();

/**
 * Enable the servo. if this is not called, the servo may not respond to SetServoEffort calls
 */
int EnableServo();
/**
 * Disable the servo. When disabled, the servo may not respond to SetServoEffort calls 
 */
int DisableServo();

/**
 * Set the servo 'effort' - a magical value between 0 and 1 that comes from the controller.
 * mapping to real servo positions will be handled by the implementation per board
 * note this should only be called from one thread
 */
int SetServoEffort(float effort);
