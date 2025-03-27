// Self Include
#include "f_core/net/network/c_ipv4.h"

// Zephyr Includes
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>

LOG_MODULE_REGISTER(CIPv4);


CIPv4::CIPv4(const char* ip, net_if *net_iface) : ip(ip), netIface(*net_iface) {
    initialize();
};

CIPv4::CIPv4(const char* ip) : CIPv4(ip, net_if_get_default()) {};

CIPv4::CIPv4(const char* ip, const device* dev) : CIPv4(ip, net_if_lookup_by_dev(dev)) {};

int CIPv4::initialize() {
    in_addr subnet{};

    err = z_impl_net_addr_pton(AF_INET, ip, &addr);
    if (err < 0) {
        LOG_ERR("Invalid IP address");
        return err;
    }

    net_if_addr const* ifaddr = net_if_ipv4_addr_add(&netIface, &addr, NET_ADDR_MANUAL, 0);
    if (!ifaddr) {
        LOG_ERR("Failed to add IP address");
        return -ENODEV;
    }

    err = z_impl_net_addr_pton(AF_INET,  CLASS_A_NETMASK, &subnet);
    if (err < 0) {
        LOG_ERR("Invalid subnet mask");
        return err;
    }

    if (!net_if_ipv4_set_netmask_by_addr(&netIface, &addr, &subnet)) {
        LOG_WRN("Failed to set subnet mask");
    }

    LOG_DBG("Initialized IPv4 address %s", ip);
    return 0;
}