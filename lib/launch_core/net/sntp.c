#include <launch_core/net/sntp.h>

#include <zephyr/kernel.h>
#include <zephyr/net/sntp.h>

void l_sntp_client_thread(void *server_ip, void *current_time, void *) {
    uint32_t *current_time_ms = (uint32_t *) current_time; // Time of day in milliseconds
    struct sntp_time sntp_time;

    while (1) {
        sntp_simple(server_ip, &sntp_time);
        *current_time_ms = (sntp_time.seconds * 1000) + (sntp_time.fraction * 1000 / 0x100000000);

        k_sleep(K_MSEC(100);
    }
}