#ifndef L_SNTP_UTILS_H
#define L_SNTP_UTILS_H

#include <sntp.h>

/**
 * Initialize an SNTP client
 * @param server_ip - IP address of the (S)NTP server
 * @param sntp_thread_id - ID of the SNTP thread. Leave -1 to not start a thread
 */
void l_sntp_client_init(const char *server_ip, k_tid_t sntp_thread_id);

/**
 * Thread for updating system time from an SNTP server
 * @param server_ip - IP address of the (S)NTP server
 * @param current_time - Pointer to the current time
 */
void l_sntp_client_thread(void *server_ip, void *current_time, void *);

#endif // L_SNTP_UTILS_H
