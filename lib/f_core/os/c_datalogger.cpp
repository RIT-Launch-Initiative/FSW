#include <f_core/os/datalogger.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(datalogger);

namespace detail {
datalogger::datalogger(const char *filename, LogMode mode, std::size_t num_packets)
    : filename(filename), mode(mode), num_packets(num_packets) {
    fs_file_t_init(&file);
    int ret = fs_open(&file, filename, FS_O_WRITE | FS_O_CREATE);

    if (ret < 0) {
        LOG_ERR("Error opening %s. %d", filename, ret);
        return;
    }
    LOG_DBG("Successfully opened %s", filename);
}
int datalogger::write(const void *data, std::size_t size) { return fs_write(&file, data, size); }

int datalogger::close() {
    LOG_DBG("Closing %s", filename);
    return fs_close(&file);
}
} // namespace detail