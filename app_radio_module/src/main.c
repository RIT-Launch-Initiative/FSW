#include <zephyr/kernel.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/uart.h>


#include <zephyr/drivers/sensor.h>
#include <app_version.h>
#include <zephyr/logging/log.h>

#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/conn_mgr_monitor.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/console/console.h>
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

void recv_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi, int8_t snr) {
	// When lora_recv_async is cancelled, may be called with 0 bytes.
        gpio_pin_toggle_dt(&led1);
	if (size != 0) {
		printk("Received %d bytes:\n\tMem View: ",size);
		for (uint16_t i = 0; i < size; i++) printk("0x%02x ",data[i]);
        printk("\nVal View: %s\n", data);
		printk("\tRSSI = %ddBm\n\tSNR = %ddBm\n", rssi, snr);

        printk("\n-----------------------------------\n");

        // send_udp_broadcast(data, size);

        memset(data, 0, size);
	} 
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



// int main() {
//     // init();
//     const struct device *lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
//     const struct device *uart_dev = DEVICE_DT_GET(DT_ALIAS(dbguart));
//     const char prompt[] = "Start typing characters to see them echoed back\r\n";
//     uint8_t tx_buff[255] = {0};
//     uint8_t tx_buff_len = 0;
//
//     printk("Starting radio module!\n");
//
//     console_init();
//     if (!device_is_ready(lora_dev)) {
//         LOG_ERR("%s not ready", lora_dev->name);
//     }
//
//
//     int ret = configure_lora(lora_dev, true);
//     if (ret != 0) {
//         printk("Error initializing LORA device. Got %d", ret);
//
//         while (1);
//     }
//     
//     printk("You should see another line with instructions below. If not,\n");
// 	printk("the (interrupt-driven) console device doesn't work as expected:\n");
// 	console_write(NULL, prompt, sizeof(prompt) - 1);    
//     
//     while (1) {
//         uint8_t character = console_getchar();
//         console_putchar(character); 
//
//         if (character == '\r') {
//             console_putchar('\n');
//             
//             ret = lora_send(lora_dev, tx_buff, tx_buff_len);
//             if (ret != 0) {
//                 printk("Error sending! Got %d\n", ret);
//             } else {
//                 printk("LoRa packet sent\n");
//             }
//
//             tx_buff_len = 0;
//         } else {
//             tx_buff[tx_buff_len++] = character;
//         }
//
//         gpio_pin_toggle_dt(&led0);
//         // gpio_pin_toggle_dt(&led1);
//         // send_udp_broadcast("Launch!", 7);
//     }
//
//     return 0;
// }
int main() {
    // init();
    const struct device *lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
    printk("Starting receiver\n");

    if (!device_is_ready(lora_dev)) {
        printk("%s not ready", lora_dev->name);
    }


    int ret = configure_lora(lora_dev, false);
    if (ret != 0) {
        printk("Error initializing LORA device. Got %d", ret);

        while (1);
    }


    char data_tx[7] = "Launch!";

    while (1) {
        ret = lora_recv_async(lora_dev, recv_cb);
    }

    return 0;
}
