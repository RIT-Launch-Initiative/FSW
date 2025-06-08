#include <f_core/os/c_datalogger.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(datalogger);

namespace detail {
datalogger::datalogger(const char *filename, LogMode mode, std::size_t num_packets)
    : filename(filename), mode(mode), num_packets(num_packets) {
    fs_file_t_init(&file);
    int ret = fs_open(&file, filename, FS_O_WRITE | FS_O_CREATE | FS_O_APPEND);

    if (ret < 0) {
        LOG_ERR("Error opening %s. %d", filename, ret);
        return;
    }
    LOG_DBG("Successfully opened %s", filename);
}
int datalogger::write(const void *data, std::size_t size) {
    LOG_INF("Writing %d bytes", size);
    if (mode == LogMode::Growing) {
        int err = fs_write(&file, data, size);
        if (err < 0) {
            LOG_ERR("Error writing to file: %d", err);
        }

        return err;
    }

    off_t offset = fs_tell(&file);
    if (offset < 0) {
        LOG_ERR("Error Seeking file: %ld", offset);
        return offset;
    }
    size_t index = offset / size;

    if (mode == LogMode::FixedSize) {
        if (index < num_packets) {
            return fs_write(&file, data, size);
        } else {
            return ENOSPC;
        }
    } else if (mode == LogMode::Circular) {
        if (index >= num_packets) {
            fs_seek(&file, 0, FS_SEEK_SET);
        }
        return fs_write(&file, data, size);
    }
    LOG_ERR("Invalid LogMode: %d", (int) mode);
    return -EINVAL;
}

int datalogger::close() {
    LOG_DBG("Closing %s", filename);
    return fs_close(&file);
}

int datalogger::sync() {
    LOG_DBG("Syncing %s", filename);
    return fs_sync(&file);
}


} // namespace detail