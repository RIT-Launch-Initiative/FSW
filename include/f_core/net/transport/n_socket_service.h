#ifndef N_SOCKET_SERVICE_H
#define N_SOCKET_SERVICE_H

#include <f_core/messaging/c_message_port.h>
#include <f_core/net/transport/c_udp_socket.h>

namespace NSocketService {

template <typename T>
int SetupUdpReceiveCallback(CUdpSocket& socket, CMessagePort<T>& messagePort) {

    socket.


}

};

#endif //N_SOCKET_SERVICE_H
