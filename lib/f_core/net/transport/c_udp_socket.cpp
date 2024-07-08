#include <f_core/net/transport/c_udp_socket.h>

#if defined(CONFIG_ARCH_POSIX)
#include <sys/socket.h>
#else
#include <zephyr/net/socket.h>
#endif
