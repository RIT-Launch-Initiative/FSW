#pragma once

#include <zephyr/posix/arpa/inet.h>

struct net_if; // Forward declaration
struct device;

class CIPv4 {
public:
    static constexpr const char* CLASS_A_NETMASK = "255.0.0.0";

    /**
     * Constructor
     * @param ip IP address to initialize
     */
    explicit CIPv4(const char* ip);

    /**
     * Constructor
     * @param ip IP address to initialize
     * @param net_iface Network interface to bind to
     */
    explicit CIPv4(const char* ip, net_if* net_iface);

    /**
     * Constructor
     * @param ip IP address to initialize
     * @param dev Device to bind to
     */
    explicit CIPv4(const char* ip, const device* dev);

    /**
     * Getter for IP address
     * @return IP address
     */
    [[nodiscard]] const char* GetIp() const { return ip; }

    /**
     * Getter for address structure
     * @return Address structure
     */
    [[nodiscard]] in_addr GetAddr() const { return addr; }

    /**
     * Getter for error code from initialization
     * @return Error code
     */
    [[nodiscard]] int GetErr() const { return err; }

    /**
     * Getter for initialization status
     * @return True if initialized, false otherwise
     */
    [[nodiscard]] bool IsInitialized() const { return err == 0; }

private:
    const char* ip;
    net_if& netIface;
    in_addr addr{};
    int err = 0;

    int initialize();
};


