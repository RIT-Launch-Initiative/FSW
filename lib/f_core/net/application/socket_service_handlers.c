#include <zephyr/net/socket_service.h>

// UDP Alert Tenant
extern void alertSocketServiceHandler(struct net_socket_service_event* pev);
NET_SOCKET_SERVICE_SYNC_DEFINE(alertSocketService, alertSocketServiceHandler, 1);
