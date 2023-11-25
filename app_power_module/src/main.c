#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>


#include "telem.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);


static struct net_if *net_interface;

// static void adc_task(void *unused0, void *unused1, void *unused2) {
//     uint16_t buff;
//     
//     struct adc_sequence adc_seq = {
//         .buffer = &buff,
//         .buffer_size = sizeof(buff)
//     };
//
//     static const struct adc_channel_cfg vin_volt_sens_channel = ADC_CHANNEL_CFG_DT(adc1);
//     // if (!adc_is_ready_dt()) {
//     //     LOG_ERR("ADC device is not ready\n");
//     //     return;
//     // }
//
//     // if (!adc_channel_setup_dt()) {
//     //     LOG_ERR("ADC channel setup failed\n");
//     //     return;
//     // }
//
//
//     while (1) {
//         int32_t tmp = 0;
//         // if (!adc_read(, &adc_seq)) {
//         //     LOG_ERR("ADC read failed\n");
//         //     continue;
//         // }
//         //
//         // if (adc_raw_to_millivolts_dt(, &tmp)) {
//             power_module_data.vin_voltage_sense = tmp;
//         k_msleep(1000);
//         // }
//     };
// }


static int init_net_stack(void) {
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


static int init(void) {
    const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);
    if (!device_is_ready(wiznet)) {
        LOG_INF("Device %s is not ready.\n", wiznet->name);
        return -ENODEV;
    } else {
        LOG_INF("Device %s is ready.\n", wiznet->name);
        init_net_stack();
    }
    
    init_ina219_tasks();
    

    // k_thread_create(&threads[3], &stacks[3][0], STACK_SIZE,
    //                  adc_task, NULL, NULL, NULL,
    //                  K_PRIO_COOP(10), 0, K_NO_WAIT);
    // k_thread_start(&threads[3]);


    return 0;
}


int main(void) {
    if (init()) {
        return -1;
    }

    while (true) {
        convert_and_send();

        k_sleep(K_MSEC(100));
    }
    return 0;
}

