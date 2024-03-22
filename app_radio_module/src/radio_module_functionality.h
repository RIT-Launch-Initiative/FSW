#ifndef RADIO_MODULE_FUNCTIONALITY_H
#define RADIO_MODULE_FUNCTIONALITY_H

#include <launch_core/backplane_defs.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/net/lora.h>
#include <launch_core/net/net_common.h>
#include <launch_core/net/udp.h>

int init_lora_unique(const struct device *lora_dev);

int init_udp_unique(l_udp_socket_list_t *udp_socket_list);

int start_tasks();

int main_unique();



#endif //RADIO_MODULE_FUNCTIONALITY_H
