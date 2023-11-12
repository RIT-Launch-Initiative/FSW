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
#include <zephyr/storage/flash_map.h>

#include <zephyr/net/net_event.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);
K_QUEUE_DEFINE(net_tx_queue);

#define STACK_SIZE (256)
static K_THREAD_STACK_ARRAY_DEFINE(ina_stacks, 3, STACK_SIZE);

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

static power_module_data_t power_module_data = {0};

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

static void tx_data(void *unused0, void *unused1, void *unused2) {
    while (true) {
        send_udp_broadcast("Launch!", 7);
//        send_udp_broadcast((const char *) &power_module_data, sizeof(power_module_data));
        k_sleep(K_MSEC(100));
    }
}


static void ina_task(void *p_id, void *unused1, void *unused2) {
    const struct device *dev;
    ina_data_t *ina_data;
    int id = (int) p_id;

    switch (id) {
        case 0:
            dev = DEVICE_DT_GET(DT_INST(0, ti_ina219));
            ina_data = &power_module_data.ina_battery;
            break;
        case 1:
            dev = DEVICE_DT_GET(DT_INST(1, ti_ina219));
            ina_data = &power_module_data.ina_3v3;
            break;
        case 2:
            dev = DEVICE_DT_GET(DT_INST(2, ti_ina219));
            ina_data = &power_module_data.ina_5v0;
            break;
        default:
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

static int init_ina219_devices() {
    const struct device *const ina_battery =  DEVICE_DT_GET(DT_INST(0, ti_ina219));
    const struct device *const ina_3v3 =  DEVICE_DT_GET(DT_INST(1, ti_ina219));
    const struct device *const ina_5v0 = DEVICE_DT_GET(DT_INST(2, ti_ina219));

    int ret = 0;
    if (device_is_ready(ina_battery)) {
        struct k_thread ina_battery_thread;
//        k_thread_create(&ina_battery_thread, ina_stacks[0], STACK_SIZE,
//                        ina_task, (void *) 0, NULL, NULL, 0, 0, K_NO_WAIT);
//        k_thread_start(&ina_battery_thread);
    } else {
        printk("Device %s is not ready.\n", ina_battery->name);
        ret |= 0b1;
    }

    if (device_is_ready(ina_3v3)) {
        struct k_thread ina_3v3_thread;
//        k_thread_create(&ina_3v3_thread, ina_stacks[1], 256,
//                        ina_task, 1, NULL, NULL, 0, 0, K_NO_WAIT);
//        k_thread_start(&ina_3v3_thread);
    } else {
        printk("Device %s is not ready.\n", ina_3v3->name);
        ret |= 0b10;
    }

    if (device_is_ready(ina_5v0)) {
        struct k_thread ina_5v0_thread;
//        k_thread_create(&ina_5v0_thread, ina_stacks[2], 256,
//                        ina_task, 2, NULL, NULL, 0, 0, K_NO_WAIT);
//        k_thread_start(&ina_5v0_thread);
    } else {
        printk("Device %s is not ready.\n", ina_5v0->name);
        ret |= 0b100;
    }

    return ret;
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


static int init(void) {
    // Queues
//    k_queue_init(&net_tx_queue);


    const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);
    if (!device_is_ready(wiznet)) {
        printk("Device %s is not ready.\n", wiznet->name);
        return -ENODEV;
    } else {
        printk("Device %s is ready.\n", wiznet->name);
        init_net_stack();
    }


    return 0;
}


int main(void) {
    if (init()) {
        while (1) {
            printf("DEADBEEF");
        }
    }

    init_ina219_devices();


    while (true) {
        send_udp_broadcast((const char *) &power_module_data, sizeof(power_module_data));
        k_sleep(K_MSEC(100));
    }
    return 0;
}

