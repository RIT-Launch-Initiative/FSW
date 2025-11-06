#ifndef C_ZMS_MANAGER_H
#define C_ZMS_MANAGER_H

#include <string>
#include <cstddef>
#include <cstdint>
#include <zephyr/fs/zms.h>

extern "C" {
struct device;
struct zms_fs;
}

class CZmsManager {
public:
    CZmsManager(device& flash, off_t offset, uint32_t sectorCount);

    ~CZmsManager() = default;

    // Read the value for `key` into `buf` up to `len` bytes. Returns number of
    // bytes read (>0) or a negative errno on error (or 0 if not found).
    ssize_t Read(const std::string &key, void *buf, size_t len);

    // Write arbitrary data for `key`. Returns number of bytes written (>=0)
    // or a negative errno on error.
    int Write(const std::string &key, const void *data, size_t len);

    // Convenience write for 32-bit integers.
    int Write(const std::string &key, uint32_t value);

    // Delete a stored key. Returns 0 on success or negative errno.
    int Delete(const std::string &key);

private:
    // Map a string key deterministically to a 32-bit ID used by ZMS.
    uint32_t IdFromKey(const std::string &key) const;

    zms_fs fs;
    bool mounted;
};

#endif //C_ZMS_MANAGER_H