/*
 * Copyright (c) 2023 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/net/net_ip.h"
#include <stdio.h>

#include <launch_core_classic/backplane_defs.h>
#include <launch_core_classic/net/net_common.h>
#include <launch_core_classic/net/udp.h>

#include <zephyr/sys/hash_map.h>
#include <zephyr/types.h>

#include <zephyr/logging/log.h>

#include <zephyr/net/socket.h>
#include <unistd.h>
#include <zephyr/net/socket.h>


LOG_MODULE_REGISTER(launch_udp_utils);

SYS_HASHMAP_DEFINE_STATIC(UDP_PORT_HANDLERS);

int l_init_udp_net_stack(struct net_if *net_interface, const char *ip_addr) {
    int ret;

    if (!net_interface) {
        LOG_INF("No network interface found");
        return -ENODEV;
    }

    struct in_addr addr;
    ret = net_addr_pton(AF_INET, ip_addr, &addr);
    if (ret < 0) {
        LOG_ERR("Invalid IP address");
        return ret;
    }

    struct net_if_addr const *ifaddr = net_if_ipv4_addr_add(net_interface, &addr, NET_ADDR_MANUAL, 0);
    if (!ifaddr) {
        LOG_ERR("Failed to add IP address");
        return -ENODEV;
    }

    struct in_addr subnet;
    ret = net_addr_pton(AF_INET, CLASS_A_NETMASK, &subnet);
    net_if_ipv4_set_netmask(net_interface, &subnet);

    net_if_set_promisc(net_interface);


    LOG_INF("IPv4 address configured: %s", ip_addr);

    return 0;
}

int l_init_udp_net_stack_default(const char *ip_addr) {
    return l_init_udp_net_stack(net_if_get_default(), ip_addr);
}

int l_init_udp_net_stack_by_device(const struct device *dev, const char *ip_addr) {
    return l_init_udp_net_stack(net_if_lookup_by_dev(dev), ip_addr);
}

int l_init_udp_socket(const char *ip, uint16_t port) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int ret = -1;

    if (sock < 0) {
        LOG_ERR("Failed to create socket (%d)", sock);
        return sock;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (ip == NULL) {
        addr.sin_addr.s_addr = INADDR_ANY;
        LOG_INF("NULL IP address received. Binding to all interfaces");
    } else {
        ret = inet_pton(AF_INET, ip, &addr.sin_addr);
        if (ret <= 0) {
            LOG_ERR("Invalid IP address format or address not specified");
            close(sock);
            return ret;
        }
    }

    ret = bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERR("Failed to bind socket (%d)", ret);
        close(sock);
        return ret;
    }

    LOG_INF("UDP socket %d bound to IP: %s, port: %d", sock, ip, port);

    return sock;
}

int l_deinit_udp_socket(int sock) {
    return close(sock);
}

int l_set_socket_rx_timeout(int sock, int timeout) {
    struct timeval time_val;
    time_val.tv_sec = timeout / 1000;
    time_val.tv_usec = (timeout % 1000) * 100;
    LOG_INF("Setting socket %d timeout to %lld seconds and %ld microseconds", sock, time_val.tv_sec, time_val.tv_usec);
    int ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &time_val, sizeof(time_val));

    if (ret < 0) {
        LOG_ERR("Failed to set socket timeout (%d)", ret);
    }

    return ret;
}

int l_send_udp_broadcast(int sock, const uint8_t *buff, size_t len, uint16_t port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    int ret = net_addr_pton(AF_INET, "255.255.255.255", &addr.sin_addr);

    if (ret == 0) {
        ret = sendto(sock, buff, len, 0, (struct sockaddr *) &addr, sizeof(addr));
        if (ret < 0) {
            LOG_ERR("Failed to send broadcast message (%d)", ret);
        }
    }

    return ret;
}

int l_receive_udp(int sock, const uint8_t *buff, size_t len) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    return recvfrom(sock, (void *) buff, len, 0, (struct sockaddr *) &addr, &addr_len);
}

int l_default_receive_thread(void *socks, void *buff_ptr, void *buff_len) {
    l_udp_socket_list_t *socket_list = (l_udp_socket_list_t *) socks;
    uint8_t *buff = (uint8_t *) buff_ptr;
    size_t len = POINTER_TO_INT(buff_len);

    while (true) {
        for (int i = 0; i < socket_list->num_sockets; i++) {
            l_receive_udp(socket_list->sockets[i], buff, len);
        }
    }

    return 0;
}

int l_add_port_handler(uint16_t port, l_udp_port_handler_t *handler) {
    return sys_hashmap_insert(&UDP_PORT_HANDLERS, port, POINTER_TO_INT(handler), NULL);
}

int l_remove_port_handler(uint16_t port) {
    return sys_hashmap_remove(&UDP_PORT_HANDLERS, port, NULL);
}