#ifndef C_IPV4_H
#define C_IPV4_H

// Forward Declares
struct net_if;
struct device;

class CIPv4 {
public:
    static constexpr const char *CLASS_A_NETMASK = "255.0.0.0";

    CIPv4(const char *ip);

    CIPv4(const char *ip, net_if net_iface);

    CIPv4(const char *ip, const device *dev);

    int Initialize();

private:
    const char *ip;
    net_if &netIface;
    bool isInitialized = false;
};



#endif //C_IPV4_H
