#pragma once

#include <stdint.h>

int l_init_event_monitor(const char *ip);

uint8_t l_get_event_udp();

uint8_t l_post_event_udp(uint8_t event);

#endif //L_EVENT_MONITOR_H
