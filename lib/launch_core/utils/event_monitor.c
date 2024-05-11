#include "launch_core/utils/event_monitor.h"

#include "launch_core/net/udp.h"
#include "launch_core/backplane_defs.h"

static int sock = -1;

int l_init_event_monitor(const char *ip) {
    return l_init_udp_socket(ip, LAUNCH_EVENT_NOTIFICATION_PORT);
}

uint8_t l_get_event_udp() {
    uint8_t notif = 0;
    if (l_receive_udp(sock, &notif, 1) == 1) {
        return notif;
    }

    return 0;
}

uint8_t l_post_event_udp(uint8_t event) {
    return l_send_udp_broadcast(sock, &event, 1, LAUNCH_EVENT_NOTIFICATION_PORT);
}