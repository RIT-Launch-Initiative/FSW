#ifndef L_SNTP_UTILS_H
#define L_SNTP_UTILS_H

#include <stdint.h>
#include <zephyr/kernel.h>


/**
 * Initialize an SNTP client
 * @param server_ip - IP address of the (S)NTP server
 * @param sntp_thread_id - ID of the SNTP thread. Leave -1 to not start a thread
 */
void l_sntp_client_init(const char *server_ip, k_tid_t sntp_thread_id);

/**
 * Start an SNTP client thread that will stop once the time has been updated
 * @param server_ip - IP address of the (S)NTP server
 * @param update_interval_ms - Interval between updates in milliseconds
 */
void l_sntp_start_client_thread(const char *server_ip, uint32_t update_interval_ms);

#endif // L_SNTP_UTILS_H
