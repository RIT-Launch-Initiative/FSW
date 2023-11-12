/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <app_version.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/fs/fs.h>
//#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
//#include <zephyr/net/socket.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/conn_mgr_monitor.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);
K_QUEUE_DEFINE(net_tx_queue);

#define STACK_SIZE (2048)
static K_THREAD_STACK_ARRAY_DEFINE(stacks, 2, STACK_SIZE);

static struct net_if *net_interface;


static int init_net_stack(void) {
    static const char ip_addr[] = "10.10.10.70";
    int ret;

    net_interface = net_if_get_default();
    if (!net_interface) {
        printk("No network interface found\n");
        return -ENODEV;
    }

    struct in_addr addr;
    ret = net_addr_pton(AF_INET, ip_addr, &addr);
    if (ret < 0) {
        printk("Invalid IP address\n");
        return ret;
    }

    struct net_if_addr *ifaddr = net_if_ipv4_addr_add(net_interface, &addr, NET_ADDR_MANUAL, 0);
    if (!ifaddr) {
        printk("Failed to add IP address\n");
        return -ENODEV;
    }

    printk("IPv4 address configured: %s\n", ip_addr);

    return 0;
}

int send_udp_broadcast(const char *data, size_t data_len) {
    int sock;
    int ret;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        printk("Failed to create socket (%d)\n", sock);
        return sock;
    }

    struct sockaddr_in dst_addr;
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(6969);
    ret = net_addr_pton(AF_INET, "255.255.255.255", &dst_addr.sin_addr);
    if (ret < 0) {
        printk("Invalid IP address format\n");
        close(sock);
        return ret;
    }

    ret = sendto(sock, data, data_len, 0, (struct sockaddr *) &dst_addr, sizeof(dst_addr));
    if (ret < 0) {
        printk("Failed to send UDP broadcast (%d)\n", ret);
        close(sock);
        return ret;
    }

    printk("Sent UDP broadcast: %s\n", data);

    close(sock);
    return 0;
}

static void init(void) {
    // Queues
//    k_queue_init(&net_tx_queue);

    // Threads
//    struct k_thread led_toggle_thread;
//    k_thread_create(&led_toggle_thread, stacks[0], STACK_SIZE,
//                    led_toggle, NULL, NULL, NULL,
//                    0, 0, K_NO_WAIT);
//    k_thread_start(&led_toggle_thread);
    init_net_stack();
}



int main(void) {
    const struct device *const ina = DEVICE_DT_GET_ONE(ti_ina219);
    struct sensor_value v_bus;
    struct sensor_value power;
    struct sensor_value current;

    init();

    if (!device_is_ready(ina)) {
        printf("Device %s is not ready.\n", ina->name);
        return 0;
    }

    while (true) {
        if (sensor_sample_fetch(ina)) {
            printf("Could not fetch sensor data.\n");
            return 0;
        }

        sensor_channel_get(ina, SENSOR_CHAN_VOLTAGE, &v_bus);
        sensor_channel_get(ina, SENSOR_CHAN_POWER, &power);
        sensor_channel_get(ina, SENSOR_CHAN_CURRENT, &current);

        printf("Bus: %f [V] -- "
               "Power: %f [W] -- "
               "Current: %f [A]\n",
               sensor_value_to_double(&v_bus),
               sensor_value_to_double(&power),
               sensor_value_to_double(&current));

        send_udp_broadcast("Launch!", 7);
        k_sleep(K_MSEC(2000));
    }

    return 0;
}

