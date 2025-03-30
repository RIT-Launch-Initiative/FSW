#ifndef N_SOCKET_SERVICE_H
#define N_SOCKET_SERVICE_H

#include <f_core/messaging/c_message_port.h>
#include <f_core/net/transport/c_udp_socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/socket.h>

static void udp_service_handler(struct net_socket_service_event* pev) {
    CMessagePort<void *> *messagePort = static_cast<CMessagePort<void *> *>(pev->user_data);
    const zsock_pollfd* pfd = &pev->event;
    int client = pfd->fd;
    sockaddr_in addr{0};
    socklen_t addrlen = sizeof(addr);
    uint8_t rcvBuff[messagePort->GetMessageSize()];

    int len = recvfrom(client, rcvBuff, sizeof(rcvBuff), 0, reinterpret_cast<struct sockaddr*>(&addr), &addrlen);
    if (len <= 0) {
        return;
    }

    messagePort->Send(rcvBuff);
}

namespace NSocketService {
    struct UserData {
        CMessagePort<void *> *messagePort;
        int messageSize;
    };

    template <typename T>
    int SetupUdpReceiveCallback(CUdpSocket& socket, CMessagePort<T>& messagePort, net_socket_service_desc serviceDesc) {
        zsock_pollfd sockPollFd[1] = {0};
        sockPollFd[0].fd = socket.GetFd();
        sockPollFd[0].events = ZSOCK_POLLIN;

        int ret = net_socket_service_register(&serviceDesc, sockPollFd, 1, messagePort);
        if (ret < 0) {
            return ret;
        }

        return 0;
    }
};

#endif //N_SOCKET_SERVICE_H
