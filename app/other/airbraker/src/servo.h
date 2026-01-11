#include <stdint.h>

/* 
 *  Call once to initialize the servo subsystem
 *  Intended for use in SYS_INIT
 */
extern "C" int servo_init();

/**
 * Set the servo 'effort' - a magical value between 0 and 1 that comes from the controller.
 *  mapping to real servo positions will be handled by the implementation per board
 * note this should only be called from one thread
 */
int set_servo_effort(float effort);
