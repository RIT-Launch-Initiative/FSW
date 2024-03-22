#ifndef RADIO_MODULE_FUNCTIONALITY_H
#define RADIO_MODULE_FUNCTIONALITY_H

#include <launch_core/backplane_defs.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/net/lora.h>
#include <launch_core/net/net_common.h>
#include <launch_core/net/udp.h>

int init_lora_unique();

int init_udp_unique();

int start_tasks();



#endif //RADIO_MODULE_FUNCTIONALITY_H
