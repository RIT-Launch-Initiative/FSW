#ifndef C_IPV4_H
#define C_IPV4_H

#include <zephyr/net/socket.h>

class CIPv4 {
public:
    CIPv4(const char *ip) : ip(ip), netIface(*net_if_get_default()) {};

    CIPv4(const char *ip, net_if net_iface) : ip(ip), netIface(net_iface) {};

    CIPv4(const char *ip, const device *dev) : ip(ip), netIface(*net_if_lookup_by_dev(dev)) {};

    int Initialize();

private:
    const char *ip;
    net_if &netIface;
    bool isInitialized = false;
};



#endif //C_IPV4_H
