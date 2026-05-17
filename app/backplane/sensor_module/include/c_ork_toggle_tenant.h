#pragma once

#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/os/c_runnable_tenant.h>

class COrkToggleTenant : public CRunnableTenant {
  public:
    /**
     * @param name Tenant name
     * @param ipStr IP address to bind the UDP socket to
     * @param port UDP port used to receive launch trigger packets
     */
    COrkToggleTenant(const char* name, const char* ipStr, int port);

    /**
     * See parent docs
     */
    void Run() override;

  private:
    CUdpSocket udp;
    int port;
};
