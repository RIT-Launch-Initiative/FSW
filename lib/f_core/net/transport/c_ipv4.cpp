#include <f_core/net/transport/c_udp_socket.h>

bool CIPv4::Initialize() {
    struct in_addr addr;
    ret = net_addr_pton(AF_INET, ip, &addr);
    if (ret < 0) {
        LOG_ERR("Invalid IP address");
        return ret;
    }

    struct net_if_addr const *ifaddr = net_if_ipv4_addr_add(net_interface, &addr, NET_ADDR_MANUAL, 0);
    if (!ifaddr) {
        LOG_ERR("Failed to add IP address");
        return -ENODEV;
    }

    struct in_addr subnet;
    ret = net_addr_pton(AF_INET, CLASS_A_NETMASK, &subnet);
    net_if_ipv4_set_netmask(net_interface, &subnet);

    net_if_set_promisc(net_interface);
    isInitialized = true;
    return 0;
}