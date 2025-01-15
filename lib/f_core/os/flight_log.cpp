#include "f_core/os/flight_log.hpp"

#include <cstdio>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(flight_log);

FlightLog::FlightLog(const char *filename) : FlightLog(filename, k_uptime_get()) {}
FlightLog::FlightLog(const char *filename, int64_t timestamp) {
    fs_file_t_init(&file);
    int err = fs_open(&file, filename, FS_O_CREATE | FS_O_APPEND);
    if (err < 0) {
        LOG_ERR("Failed to open flight log: %d", err);
        return;
    }
    // Write openning message
    err = Write("FlightLog opened");
    if (err < 0) {
        LOG_ERR("Failed to write opening message to flight log");
    }
}

FlightLog::~FlightLog() {
    int res = Close();
    if (res < 0) {
        LOG_ERR("Error closing file: %d", res);
    }
}

int FlightLog::Write(const char *msg) { return Write(k_uptime_get(), msg); }
int FlightLog::Write(int64_t timestamp, const char *msg) {
    int str_len = strlen(msg);
    return Write(k_uptime_get(), msg, str_len);
}

int FlightLog::Write(const char *msg, size_t str_len) { return Write(k_uptime_get(), msg, str_len); }

int FlightLog::Write(int64_t timestamp, const char *msg, size_t str_len) {
    LOG_INF("message: %9lld: %s", timestamp, msg);
    int ret = writeTimestamp(timestamp);
    if (ret < 0) {
        LOG_ERR("Error writing timestamp: %d", ret);
        return ret;
    }

    ret = fs_write(&file, (void *) msg, str_len);
    if (ret < 0) {
        LOG_ERR("Error writing message: %d", ret);
        return ret;
    }
    char nl = '\n';
    ret = fs_write(&file, (void *) &nl, 1);
    if (ret < 0) {
        LOG_ERR("Error writing newline: %d", ret);
        return ret;
    };
    return 0;
}

int FlightLog::Sync() { return fs_sync(&file); }
int FlightLog::Close() { return fs_close(&file); }

int FlightLog::writeTimestamp(int64_t timestamp) {
    constexpr size_t buf_size = 12; // max int64 is 19 + negative sign just in case + ': '
    char buf[buf_size] = {0};
    int len = snprintf(buf, buf_size, "%9lld: ", timestamp); // 9 digit padding keeps alignment until a week and a half
    return fs_write(&file, (void *) buf, len);
}
