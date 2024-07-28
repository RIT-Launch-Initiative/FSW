#ifndef C_IPV4_H
#define C_IPV4_H

#include <zephyr/posix/arpa/inet.h>

struct net_if; // Forward declaration
struct device;

class CIPv4
{
public:
    static constexpr const char* CLASS_A_NETMASK = "255.0.0.0";

    explicit CIPv4(const char* ip);
    CIPv4(const char* ip, net_if* net_iface);
    CIPv4(const char* ip, const device* dev);


    [[nodiscard]] const char* GetIp() const { return ip; }

    [[nodiscard]] const in_addr& GetAddr() const { return addr; }

    [[nodiscard]] const int GetErr() const { return err; }

    [[nodiscard]] const int IsInitialized() const { return err == 0; }

private:
    const char* ip;
    net_if& netIface;
    in_addr addr{};
    int err = 0;

    int initialize();
};

#endif
