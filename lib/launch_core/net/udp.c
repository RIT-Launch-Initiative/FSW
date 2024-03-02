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
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/net/socket.h>


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

    struct in_addr subnet;
    ret = net_addr_pton(AF_INET, "255.255.255.0", &subnet);
    net_if_ipv4_set_netmask(net_interface, &subnet);

    net_if_set_promisc(net_interface);


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
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int ret = recvfrom(sock, (void *) buff, len, 0, (struct sockaddr *) &addr, &addr_len);
    if (ret < 0) {
        LOG_ERR("Failed to receive data (%d)\n", ret);
    } else {
        LOG_INF("Received %d bytes from %d: %s", ret, ntohs(addr.sin_port), buff);
    }

    return 0;
}

int l_add_port_handler(uint16_t port, l_udp_port_handler_t *handler) {
    return sys_hashmap_insert(&UDP_PORT_HANDLERS, port, POINTER_TO_INT(handler), NULL);
}

int l_remove_port_handler(uint16_t port) {
    return sys_hashmap_remove(&UDP_PORT_HANDLERS, port, NULL);
}


void l_receive_multicast_packets(int port, uint8_t *buffer, size_t buffer_size) {
    int sockfd;
    struct sockaddr_in my_addr;
    struct ip_mreqn mreq;



    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        LOG_ERR("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Enable SO_REUSEADDR to allow multiple instances of this application to receive copies of the multicast datagrams
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
        LOG_ERR("setting SO_REUSEADDR error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Set up the local address and port to bind to
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // Receive multicast on any interface
    my_addr.sin_port = htons(port);

    // Bind socket to the local address
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        LOG_ERR("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Join multicast group
    mreq.imr_multiaddr.s4_addr[0] = 224;  // Multicast group address
    mreq.imr_multiaddr.s4_addr[1] = 0;  // Multicast group address
    mreq.imr_multiaddr.s4_addr[2] = 0;  // Multicast group address
    mreq.imr_multiaddr.s4_addr[3] = 1;  // Multicast group address

    mreq.imr_address.s_addr = htonl(INADDR_ANY);       // Use default interface
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
        LOG_ERR("setsockopt");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    LOG_INF("Waiting for multicast packets...\n");

    ssize_t num_bytes = recvfrom(sockfd, buffer, buffer_size, 0, NULL, NULL);
    if (num_bytes < 0) {
        LOG_ERR("recvfrom");
        close(sockfd);
    }
    buffer[num_bytes] = '\0'; // Null-terminate the received data
    LOG_INF("Received multicast packet: %s\n", buffer);

    close(sockfd);
}

int l_udp_receive(int port, uint8_t *buffer, size_t buffer_size) {
    struct sockaddr_in addr;
    int sock, len, rc;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        LOG_ERR("Failed to create socket\n");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rc = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (rc < 0) {
        LOG_ERR("Failed to bind socket\n");
        close(sock);
        return -1;
    }

    LOG_INF("Waiting for data...\n");
    len = recv(sock, buffer, buffer_size, 0);
    if (len < 0) {
        LOG_ERR("Error receiving data\n");
        close(sock);
        return -1;
    }

    buffer[len] = '\0';

    close(sock);

    return len;
}