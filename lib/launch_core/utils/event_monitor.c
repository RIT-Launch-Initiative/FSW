#include "launch_core/utils/event_monitor.h"

#include "launch_core/net/udp.h"
#include "launch_core/backplane_defs.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(event_monitor);

static int sock = -1;

int l_init_event_monitor(const char* ip) {
    sock = l_init_udp_socket(ip, LAUNCH_EVENT_NOTIFICATION_PORT);
    if (sock < 0) {
        sock = -1;
    }

    int ret = l_set_socket_rx_timeout(sock, 1);
    if (ret == -1) {
        return -1;
    }

    return sock;
}

uint8_t l_get_event_udp() {
    if (sock == -1) {
        LOG_ERR("Socket for port 9999 uninitialized!");
        return 0;
    }

    uint8_t notif = 0;
    if (l_receive_udp(sock, &notif, 1) == 1) {
        return notif;
    }

    return 0;
}

uint8_t l_post_event_udp(uint8_t event) {
    if (sock == -1) {
        LOG_ERR("Socket for port 9999 uninitialized!");
        return 0;
    }

    return l_send_udp_broadcast(sock, &event, 1, LAUNCH_EVENT_NOTIFICATION_PORT);
}