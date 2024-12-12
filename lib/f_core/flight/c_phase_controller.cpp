#include "f_core/flight/c_phase_controller.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flight_controller);

int flight_log_init(const char *flight_log_file_name, fs_file_t *fp) {
    if (flight_log_file_name == nullptr) {
        LOG_INF("Not initializing flight log. No file name supplied");
        return 0;
    }
    LOG_INF("Beginning Flight Log");
    fs_file_t_init(fp);
    int err = fs_open(fp, flight_log_file_name, FS_O_CREATE | FS_O_APPEND);
    if (err != 0) {
        return 0;
    }
    constexpr size_t buf_size = 64;
    char string_buf[buf_size] = {0};
    int num_wrote = snprintf(string_buf, buf_size, "Flight Log Opened at %lld ms\n", k_uptime_get());

    return fs_write(fp, (void *) string_buf, num_wrote);
}
int flight_log_source_event(bool enabled, const char *source, const char *event) {
    LOG_INF("%lldms: %-10s from %s", k_uptime_get(), event, source);
    if (!enabled) {
        return 0;
    }
    return 0;
}
int flight_log_event_confirmed(bool enabled, const char *event, bool currentState) {
    LOG_INF("%lldms: %-10s confirmed%s", k_uptime_get(), event,
            currentState ? " but already happened. Not dispatching" : "");
    if (!enabled) {
        return 0;
    }
    return 0;
}