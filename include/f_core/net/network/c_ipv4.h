#ifndef C_IPV4_H
#define C_IPV4_H

#if defined(CONFIG_ARCH_POSIX)
#include <arpa/inet.h>
#else
#include <zephyr/posix/arpa/inet.h>
#endif

struct net_if;
// Forward Declares
struct device;

class CIPv4 {
public:
    static constexpr const char *CLASS_A_NETMASK = "255.0.0.0";

    explicit CIPv4(const char *ip);
    CIPv4(const char *ip, net_if *net_iface);
    CIPv4(const char *ip, const device *dev);

    int Initialize();

    const char *GetIp() const { return ip; }

    const in_addr &GetAddr() const { return addr; }

private:
    const char *ip;
    net_if &netIface;
    bool isInitialized = false;
    in_addr addr{};
};



#endif //C_IPV4_H
