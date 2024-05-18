#ifndef SENSOR_MODULE_H
#define SENSOR_MODULE_H

#include <launch_core/net/net_common.h>

#define SENSOR_MODULE_IP_ADDR BACKPLANE_IP(SENSOR_MODULE_ID, 2, 1)

typedef enum {
    PAD_STATE = 0,
    PRE_MAIN_STATE,
    POST_MAIN_STATE,
    LANDING_STATE
} FLIGHT_STATES;

int init_networking(void);

#endif //SENSOR_MODULE_H
