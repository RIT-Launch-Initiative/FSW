#include <zephyr/kernel.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/drivers/lora.h>

#include <zephyr/drivers/sensor.h>
#include <app_version.h>
#include <zephyr/logging/log.h>

#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/conn_mgr_monitor.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>

#include <zephyr/drivers/gpio.h>

#define SLEEP_TIME_MS   100
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);
K_QUEUE_DEFINE(lora_tx_queue);
K_QUEUE_DEFINE(net_tx_queue);

static struct net_if *net_iface;

int init_net_stack(void) {
    static const char ip_addr[] = "10.10.10.69";
    int ret;

    net_iface = net_if_get_default();
    if (!net_iface) {
        printk("No network interface found\n");
        return -ENODEV;
    }

    struct in_addr addr;
    ret = net_addr_pton(AF_INET, ip_addr, &addr);
    if (ret < 0) {
        printk("Invalid IP address\n");
        return ret;
    }

    struct net_if_addr *ifaddr = net_if_ipv4_addr_add(net_iface, &addr, NET_ADDR_MANUAL, 0);
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
    k_queue_init(&lora_tx_queue);
    k_queue_init(&net_tx_queue);

    // Devices
    const struct device *const sx1276 = DEVICE_DT_GET_ONE(semtech_sx1276);
    if (!device_is_ready(sx1276)) {
        printk("Device %s is not ready.\n", sx1276->name);
    } else {
        printk("Device %s is ready.\n", sx1276->name);
    }

    const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);
    if (!device_is_ready(wiznet)) {
        printk("Device %s is not ready.\n", wiznet->name);
    } else {
        printk("Device %s is ready.\n", wiznet->name);
    }

    init_net_stack();
}

static void get_gnss(void) {

}

static int configure_lora(const struct device *dev, bool transmit) {
    struct lora_modem_config config = {
        .frequency = 915000000,
        .bandwidth = BW_125_KHZ,
        .datarate = SF_10,
        .preamble_len = 8,
        .coding_rate = CR_4_5,
        .tx_power = 4,
        .iq_inverted = false,
        .public_network = false,
        .tx = transmit
    };

    return lora_config(dev, &config);
}


static void lora_tx() {
    // if (!k_queue_is_empty(&lora_tx_queue)) {
    //
    //
    // }
}

static void lora_rx() {
}

static void toggle_led() {

}

int main() {
    // init();
    printk("Starting radio module!");
    const struct device *lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));

    if (!device_is_ready(lora_dev)) {
        LOG_ERR("%s not ready", lora_dev->name);
    }


    int ret = configure_lora(lora_dev, true);
    if (ret != 0) {
        printk("Error initializing LORA device. Got %d", ret);

        while (1);
    }


    char data_tx[7] = "Launch!";
    while (1) {
        ret = lora_send(lora_dev, data_tx, sizeof(data_tx));
        if (ret != 0) {
            printk("Error sending! Got %d\n", ret);
        } else {
            printk("LoRa packet sent\n");
        }
    
        // gpio_pin_toggle_dt(&led0);
        // gpio_pin_toggle_dt(&led1);
        // send_udp_broadcast("Launch!", 7);
        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
