#include "f_core/flight/flight_log.hpp"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(flight_log);

int FlightLog::Write(const char *msg) { return Write(k_uptime_get(), msg); }
int FlightLog::Write(int64_t timestamp, const char *msg) {
    int ret = writeTimestamp(timestamp);
    if (ret < 0) {
        LOG_ERR("Error writing timestamp: %d", ret);
        return ret;
    }

    int len = strlen(msg);
    return fs_write(&file, msg, )
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
