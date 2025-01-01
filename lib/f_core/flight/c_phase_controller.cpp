#include "f_core/flight/c_phase_controller.h"

#include <string.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flight_controller);

int flight_log_print(bool enabled, fs_file_t *fp, const char *str) {
    LOG_INF("%7lldms:\tFlight Log Print: %s", k_uptime_get(), str);
    if (!enabled) {
        return 0;
    }
    int len = strnlen(str, 1024);

    int err = fs_write(fp, (void *) str, len);
    if (err == -EBADF) {
        LOG_WRN("Log file already closed");
        return 0;
    } else if (err < 0) {
        LOG_ERR("Failed to print to flight log");
        return err;
    }
    char nl = '\n';
    err = fs_write(fp, (void *) &nl, 1);
    if (err == -EBADF) {
        LOG_WRN("Log file already closed");
        return 0;
    } else if (err < 0) {
        LOG_ERR("Failed to add newline to flight log");
        return err;
    }

    return 0;
}

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
    int num_wrote = snprintf(string_buf, buf_size, "%7lldms:\tFlight log opened\n", k_uptime_get());

    return fs_write(fp, (void *) string_buf, num_wrote);
}

int flight_log_close(bool enabled, fs_file_t *fp) {
    if (!enabled) {
        return 0;
    }
    LOG_INF("%7lldms:\tClosing flight log", k_uptime_get());
    constexpr size_t buf_size = 64;
    char string_buf[buf_size] = {0};
    int num_wrote = snprintf(string_buf, buf_size, "%7lldms:\tClosing flight log\n", k_uptime_get());
    int err = fs_write(fp, (void *) string_buf, num_wrote);
    if (err == -EBADF) {
        LOG_WRN("Log file already closed");
        return 0;
    }
    if (err < 0) {
        LOG_ERR("Error writing closing message to flight log: %d", err);
        return err;
    }
    return fs_close(fp);
}

int flight_log_source_event(bool enabled, fs_file_t *fp, const char *source, const char *event) {
    LOG_INF("%7lldms:\t%-10s from %s", k_uptime_get(), event, source);
    if (!enabled) {
        return 0;
    }
    constexpr size_t buf_size = 64;
    char string_buf[buf_size] = {0};
    int num_wrote = snprintf(string_buf, buf_size, "%7lldms:\t%-10s from %s\n", k_uptime_get(), event, source);

    int err = fs_write(fp, (void *) string_buf, num_wrote);
    if (err == -EBADF) {
        LOG_WRN("Log file already closed");
        return 0;
    } else if (err < 0) {
        LOG_ERR("Error writing message to flight log: %d", err);
    }

    return 0;
}
int flight_log_event_confirmed(bool enabled, fs_file_t *fp, const char *event, bool currentState) {
    LOG_INF("%7lldms:\t%-10s confirmed%s", k_uptime_get(), event,
            currentState ? " but already happened. Not dispatching" : "");
    if (!enabled) {
        return 0;
    }
    constexpr size_t buf_size = 128;
    char string_buf[buf_size] = {0};
    int num_wrote = snprintf(string_buf, buf_size, "%7lldms:\t%-10s confirmed%s\n", k_uptime_get(), event,
                             currentState ? " but already happened. Not dispatching" : "");

    int err = fs_write(fp, (void *) string_buf, num_wrote);
    if (err == -EBADF) {
        LOG_WRN("Log file already closed");
        return 0;
    } else if (err < 0) {
        LOG_ERR("Error writing message to flight log: %d", err);
    }

    return 0;
}