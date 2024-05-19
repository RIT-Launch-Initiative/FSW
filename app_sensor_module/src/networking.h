#ifndef SENSOR_MODULE_NETWORKING_H
#define SENSOR_MODULE_NETWORKING_H

#include <launch_core/backplane_defs.h>
#include <launch_core/net/net_common.h>
#include <launch_core/net/udp.h>

#define SENSOR_MODULE_IP_ADDR BACKPLANE_IP(SENSOR_MODULE_ID, 2, 1) // TODO: KConfig the board revision and #

#endif //SENSOR_MODULE_NETWORKING_H
