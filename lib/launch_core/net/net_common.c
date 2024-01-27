#include <launch_core/net/net_common.h>

#include <stdio.h>



int l_create_ip_str(char *ip_str, int a, int b, int c, int d) {
    if (!ip_str) return -5;
    if (a < 0 || a > 255) return -1;
    if (b < 0 || b > 255) return -2;
    if (c < 0 || c > 255) return -3;
    if (d < 0 || d > 255) return -4;

    return snprintf(ip_str, MAX_IP_ADDRESS_STR_LEN, "%d.%d.%d.%d", a, b, c, d);
}

int l_create_ip_str_default_net_id(char *ip_str, int c, int d) {
    return l_create_ip_str(ip_str, BACKPLANE_NETWORK_ID[0], BACKPLANE_NETWORK_ID[1], c, d);
}