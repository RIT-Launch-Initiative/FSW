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

typedef struct ina_data {
    struct sensor_value current;
    struct sensor_value voltage;
    struct sensor_value power;
} ina_data_t;

typedef struct {
    ina_data_t ina_battery;
    ina_data_t ina_3v3;
    ina_data_t ina_5v0;
} power_module_data_t;

static void ina_task(void *device, void *data, void *unused2) {
    const struct device *const dev = (const struct device *) device;
    ina_data_t *ina_data = (ina_data_t *) data;
    
    if (!device_is_ready(dev)) {
        printk("Device %s is not ready.\n", dev->name);
        return;
    }

    while (true) {
        sensor_sample_fetch(dev);
        sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE, &ina_data->voltage);
        sensor_channel_get(dev, SENSOR_CHAN_POWER, &ina_data->power);
        sensor_channel_get(dev, SENSOR_CHAN_CURRENT, &ina_data->current);
        k_sleep(K_MSEC(100));
    }
}

static int init_net_stack(void) {
    static const char ip_addr[] = "10.10.10.69";
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

static void tx_data(void *power_module_data, void *unused1, void *unused2) {
    while (true) {
        send_udp_broadcast((const char *) power_module_data, sizeof(power_module_data));
        k_sleep(K_MSEC(100));
    }
}

static int init(void) {
    // Queues
//    k_queue_init(&net_tx_queue);

    // Threads
//    struct k_thread led_toggle_thread;
//    k_thread_create(&led_toggle_thread, stacks[0], STACK_SIZE,
//                    led_toggle, NULL, NULL, NULL,
//                    0, 0, K_NO_WAIT);
//    k_thread_start(&led_toggle_thread);

    const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);
    if (!device_is_ready(wiznet)) {
        printk("Device %s is not ready.\n", wiznet->name);
    } else {
        printk("Device %s is ready.\n", wiznet->name);
        return init_net_stack();
    }

    return -1;
}


int main(void) {
    if (init()) {
        while (1) {
            printf("DEADBEEF");
        }
    }

    while (1) {
        send_udp_broadcast("Launch!", 7);
    }
    return 0;
}

