#include <zephyr/net/socket_service.h>

extern void alertSocketServiceHandler(net_socket_service_desc* svc, void* userData);
NET_SOCKET_SERVICE_SYNC_DEFINE(alertSocketService, alertSocketServiceHandler, 1);
