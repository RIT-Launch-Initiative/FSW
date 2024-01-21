/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <launch_core/net_utils.h>
#include <zephyr/sys/hash_map.h>
#include <zephyr/types.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(net_utils);

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

int l_send_udp_broadcast(const uint8_t *data, size_t data_len, uint16_t port) {
    int sock;
    int ret;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        LOG_ERR("Failed to create socket (%d)\n", sock);
        return sock;
    }

    struct sockaddr_in dst_addr;
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(port);
    ret = net_addr_pton(AF_INET, "255.255.255.255", &dst_addr.sin_addr);
    if (ret < 0) {
        LOG_ERR("Invalid IP address format\n");
        close(sock);
        return ret;
    }

    ret = sendto(sock, data, data_len, 0, (struct sockaddr *) &dst_addr, sizeof(dst_addr));
    if (ret < 0) {
        LOG_ERR("Failed to send UDP broadcast (%d)\n", ret);
        close(sock);
        return ret;
    }

    LOG_INF("Sent UDP broadcast: %s\n", data);

    close(sock);
    return 0;
}

int l_receive_udp_callback(const struct device *dev, struct net_pkt *packet, int status) {
    // TODO: Currently being implemented and tested in another branch
    if (sys_hashmap_contains_key(&UDP_PORT_HANDLERS, &packet->port)) {
        l_udp_port_handler_t *port_handler = sys_hashmap_get(&UDP_PORT_HANDLERS, &packet->port);
        port_handler->handler(NULL, NULL);
    }

    return 0;
}

int l_add_port_handler(uint16_t port, void (*handler)(uint8_t *data, size_t data_len)) {
    return sys_hashmap_insert(&UDP_PORT_HANDLERS, &port, handler, NULL);
}

int l_remove_port_handler(uint16_t port) {
    return sys_hashmap_remove(&UDP_PORT_HANDLERS, &port, NULL);
}
