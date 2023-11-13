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

#define STACK_SIZE (2048)
static K_THREAD_STACK_ARRAY_DEFINE(ina_stacks, 3, STACK_SIZE);
static K_THREAD_STACK_DEFINE(adc_stack, STACK_SIZE);

static struct net_if *net_interface;

typedef struct {
    struct sensor_value current;
    struct sensor_value voltage;
    struct sensor_value power;
    int16_t vin_voltage_sense;
} ina_data_t;

typedef struct {
    ina_data_t ina_battery;
    ina_data_t ina_3v3;
    ina_data_t ina_5v0;
} power_module_data_t;


typedef struct __attribute__((__packed__)) {
    float current_battery;
    float voltage_battery;
    float power_battery;
    
    float current_3v3;
    float voltage_3v3;
    float power_3v3;

    float current_5v0;
    float voltage_5v0;
    float power_5v0;

    int16_t vin_voltage_sense;
} power_module_packet_t;

static power_module_data_t power_module_data = {0};
static struct k_thread ina_threads[3] = {0};
static struct k_thread adc_thread = {};

int send_udp_broadcast(const uint8_t *data, size_t data_len) {
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

static void adc_task(void *unused0, void *unused1, void *unused2) {

    while (1) {

    };
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

static void init_ina219_tasks() {
    for (int i = 0; i < 3; i++) {
        k_thread_create(&ina_threads[i], &ina_stacks[i][0], STACK_SIZE,
                        ina_task, INT_TO_POINTER(i), NULL, NULL,
                        K_PRIO_COOP(10), 0, K_NO_WAIT);

        k_thread_start(&ina_threads[i]);
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
    
    k_thread_create(&adc_thread, adc_stack, STACK_SIZE,
                    adc_task, NULL, NULL, NULL,
                    K_PRIO_COOP(10), 0, K_NO_WAIT);
    k_thread_start(&adc_thread);
   
   


    init_ina219_tasks();

    

    return 0;
}


int main(void) {
    if (init()) {
        while (1) {
            printf("DEADBEEF");
        }
    }


    power_module_packet_t packet = {0};

    while (true) {
        packet.current_battery =  sensor_value_to_float(&power_module_data.ina_battery.current);
        packet.voltage_battery = sensor_value_to_float(&power_module_data.ina_battery.voltage);
        packet.power_battery = sensor_value_to_float(&power_module_data.ina_battery.power);

        packet.current_3v3 = sensor_value_to_float(&power_module_data.ina_3v3.current);
        packet.voltage_3v3 = sensor_value_to_float(&power_module_data.ina_3v3.voltage);
        packet.power_3v3 = sensor_value_to_float(&power_module_data.ina_3v3.power);

        packet.current_5v0 = sensor_value_to_float(&power_module_data.ina_5v0.current);
        packet.voltage_5v0 = sensor_value_to_float(&power_module_data.ina_5v0.voltage);
        packet.power_5v0 = sensor_value_to_float(&power_module_data.ina_5v0.power);
        packet.vin_voltage_sense = 0xBEEF;

        send_udp_broadcast((const uint8_t *) &packet, sizeof(power_module_packet_t));
        k_sleep(K_MSEC(100));
    }
    return 0;
}

