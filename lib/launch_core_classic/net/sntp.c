#include <launch_core_classic/net/sntp.h>
#include <launch_core_classic/os/time.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/sntp.h>

#include <stdint.h>

LOG_MODULE_REGISTER(sntp_utils);

/**
 * Thread to update the current time from an (S)NTP server. Stops after updating the time once.
 * @param server_ip - IP address of the (S)NTP server
 * @param update_interval_ms - Interval between updates in milliseconds
 * @param timeout - Timeout in milliseconds
 */
static void l_sntp_client_thread(void *p_server_ip, void *timeout, void *) {
    const char *server_ip = (char *) p_server_ip;
    struct sntp_time sntp_time;
    uint64_t timeout_ms = POINTER_TO_UINT(timeout);

    LOG_INF("Starting SNTP client thread. Server IP: %s", server_ip);

    while (1) {
        if (sntp_simple(server_ip, timeout_ms, &sntp_time) == 0) {
            uint32_t time_of_day_ms = (sntp_time.seconds * 1000) + (sntp_time.fraction * 1000 / 0x100000000) ;
            l_init_time(time_of_day_ms);
            break;
        }
    }

    LOG_INF("SNTP client thread finished. Time of day: %u", l_get_time_of_day_ms(k_uptime_get_32()));
}

void l_sntp_start_client_thread(const char *server_ip, uint64_t timeout_ms) {
    // TODO: Dynamically allocate the stack and have a callback to free it
    static K_THREAD_STACK_DEFINE(sntp_thread_stack, SNTP_CLIENT_STACK_SIZE);
    static struct k_thread sntp_thread;

    k_tid_t sntp_thread_id = k_thread_create(&sntp_thread, sntp_thread_stack, SNTP_CLIENT_STACK_SIZE,
                                          l_sntp_client_thread, (void *) server_ip, UINT_TO_POINTER(timeout_ms), NULL,
                                          0, 0, K_NO_WAIT);
    k_thread_start(sntp_thread_id);
}

