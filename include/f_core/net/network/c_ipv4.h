#ifndef C_IPV4_H
#define C_IPV4_H

#include <zephyr/net/socket.h>

struct net_if; // Forward declaration
struct device;

class CIPv4 {
public:
    static constexpr const char* CLASS_A_NETMASK = "255.0.0.0";

    explicit CIPv4(const char* ip);
    CIPv4(const char* ip, net_if* net_iface);
    CIPv4(const char* ip, const device* dev);

    int Initialize();

    [[nodiscard]] const char* GetIp() const { return ip; }

    [[nodiscard]] const in_addr& GetAddr() const { return addr; }

private:
    const char* ip;
    net_if& netIface;
    bool isInitialized = false;
    in_addr addr{};
};

#endif
