#ifndef C_IPV4_H
#define C_IPV4_H


struct in_addr;
struct net_if;
// Forward Declares
struct device;

class CIPv4 {
public:
    static constexpr const char *CLASS_A_NETMASK = "255.0.0.0";

    CIPv4(const char *ip);

    CIPv4(const char *ip, net_if net_iface);

    CIPv4(const char *ip, const device *dev);

    int Initialize();

    const char *GetIp() const { return ip; }

    const in_addr &GetAddr() const { return *addr; }

private:
    const char *ip;
    in_addr *addr;
    net_if &netIface;
    bool isInitialized = false;
};



#endif //C_IPV4_H
