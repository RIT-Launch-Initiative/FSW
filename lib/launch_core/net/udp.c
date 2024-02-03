/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <launch_core/backplane_defs.h>
#include <launch_core/net/udp.h>

#include <zephyr/sys/hash_map.h>
#include <zephyr/types.h>

#include <zephyr/logging/log.h>

#include <zephyr/net/socket.h>
#include <unistd.h>

LOG_MODULE_REGISTER(launch_udp_utils);

SYS_HASHMAP_DEFINE_STATIC(UDP_PORT_HANDLERS);

static struct net_if *net_interface;

int l_init_udp_net_stack(const char *ip_addr) {
    int ret;

    net_interface = net_if_get_default();
    if (!net_interface) {
        LOG_INF("No network interface found\n");
        return -ENODEV;
    }

    struct in_addr addr;
    ret = net_addr_pton(AF_INET, ip_addr, &addr);
    if (ret < 0) {
        LOG_ERR("Invalid IP address\n");
        return ret;
    }

    struct net_if_addr *ifaddr = net_if_ipv4_addr_add(net_interface, &addr, NET_ADDR_MANUAL, 0);
    if (!ifaddr) {
        LOG_ERR("Failed to add IP address\n");
        return -ENODEV;
    }

    LOG_INF("IPv4 address configured: %s\n", ip_addr);

    return 0;
}

int l_init_udp_socket(const char *ip, uint16_t port) {
    static const int broadcast_enable = 1;
    int ret = -1;
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));

    if (sock < 0) {
        LOG_ERR("Failed to create socket (%d)\n", sock);
        return sock;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (ip == NULL) {
        addr.sin_addr.s_addr = INADDR_ANY;
        LOG_INF("NULL IP address received. Binding to all interfaces\n");
    } else {
        ret = inet_pton(AF_INET, ip, &addr.sin_addr);
        if (ret <= 0) {
            LOG_ERR("Invalid IP address format or address not specified\n");
            close(sock);
            return ret;
        }
    }

    ret = bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERR("Failed to bind socket (%d)\n", ret);
        close(sock);
        return ret;
    }

    LOG_INF("UDP socket bound to IP: %s, port: %d\n", ip, port);

    return 0;
}

int l_deinit_udp_socket(int sock) {
    return close(sock);
}

int l_send_udp_broadcast(int sock, const uint8_t *buff, size_t len, uint16_t port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    int ret = net_addr_pton(AF_INET, "255.255.255.255", &addr.sin_addr);

    if (ret == 0) {
        ret = sendto(sock, buff, len, 0, (struct sockaddr *) &addr, sizeof(addr));
        if (ret < 0) {
            LOG_ERR("Failed to send broadcast message (%d)\n", ret);
        }
    }

    return ret;
}

int l_receive_udp_poll(int sock, const uint8_t *buff, size_t len, uint16_t port) {

    return 0;
}

int l_add_port_handler(uint16_t port, l_udp_port_handler_t *handler) {
    return sys_hashmap_insert(&UDP_PORT_HANDLERS, port, POINTER_TO_INT(handler), NULL);
}

int l_remove_port_handler(uint16_t port) {
    return sys_hashmap_remove(&UDP_PORT_HANDLERS, port, NULL);
}
