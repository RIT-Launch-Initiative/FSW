#include <zephyr/net/socket_service.h>

// SNTP Server Tenant
extern void sntpSocketServiceHandler(struct net_socket_service_event* pev);
NET_SOCKET_SERVICE_SYNC_DEFINE(sntpSocketService, sntpSocketServiceHandler, 1);

// TFTP Server Tenant
extern void tftpSocketServiceHandler(struct net_socket_service_event* pev);
NET_SOCKET_SERVICE_SYNC_DEFINE(tftpSocketService, tftpSocketServiceHandler, 1);

// UDP Alert Tenant
extern void alertSocketServiceHandler(struct net_socket_service_event* pev);
NET_SOCKET_SERVICE_SYNC_DEFINE(alertSocketService, alertSocketServiceHandler, 1);
