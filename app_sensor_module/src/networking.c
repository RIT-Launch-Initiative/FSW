#include "networking.h"

#include <zephyr/kernel.h>


static void telemetry_broadcast_task(void *, void *, void *);

K_THREAD_DEFINE(telemetry_broadcast, 1024, telemetry_broadcast_task, NULL, NULL, NULL, 10, 0, 0);

extern struct k_msgq hundred_hz_telem_queue;

static void telemetry_broadcast_task(void *, void *, void *) {
    while (true) {

    }
}