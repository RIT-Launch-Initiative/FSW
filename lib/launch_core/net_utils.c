/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <launch_core/net_utils.h>

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_event.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(net_utils, CONFIG_APP_LOG_LEVEL);

static struct net_if *net_interface;

int init_eth_iface(const struct device *dev) {
    if (!device_is_ready(dev)) {
        LOG_INF("Device %s is not ready.\n", dev->name);
        return -ENODEV;
    } 
    
    LOG_INF("Device %s is ready.\n", dev->name);
    return 0;
}

int init_net_stack(void) {
    static const char ip_addr[] = "10.10.10.69";
    int ret;

    net_interface = net_if_get_default();
    if (!net_interface) {
        LOG_INF("No network interface found\n");
        return -ENODEV;
    }

    struct in_addr addr;
    ret = net_addr_pton(AF_INET, ip_addr, &addr);
    if (ret < 0) {
        LOG_INF("Invalid IP address\n");
        return ret;
    }

    struct net_if_addr *ifaddr = net_if_ipv4_addr_add(net_interface, &addr, NET_ADDR_MANUAL, 0);
    if (!ifaddr) {
        LOG_INF("Failed to add IP address\n");
        return -ENODEV;
    }

    LOG_INF("IPv4 address configured: %s\n", ip_addr);

    return 0;
}

int send_udp_broadcast(const uint8_t *data, size_t data_len, uint16_t port) {
    int sock;
    int ret;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        LOG_INF("Failed to create socket (%d)\n", sock);
        return sock;
    }

    struct sockaddr_in dst_addr;
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(port);
    ret = net_addr_pton(AF_INET, "255.255.255.255", &dst_addr.sin_addr);
    if (ret < 0) {
        LOG_INF("Invalid IP address format\n");
        close(sock);
        return ret;
    }

    ret = sendto(sock, data, data_len, 0, (struct sockaddr *) &dst_addr, sizeof(dst_addr));
    if (ret < 0) {
        LOG_INF("Failed to send UDP broadcast (%d)\n", ret);
        close(sock);
        return ret;
    }

    LOG_INF("Sent UDP broadcast: %s\n", data);

    close(sock);
    return 0;
}
