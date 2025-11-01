#include "f_core/os/c_zms_manager.h"

extern "C" {
#include <zephyr/fs/zms.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/__assert.h>
}

#include <cstring>

CZmsManager::CZmsManager()
    : _fs(nullptr), _mounted(false) {}

CZmsManager::~CZmsManager() {
    if (_mounted) {
        Deinit();
    }
}

int CZmsManager::Init(struct device *flash_device, off_t offset, uint32_t sector_count) {
    if (!flash_device) {
        return -EINVAL;
    }

    // Allocate zms_fs
    _fs = new zms_fs();
    if (!_fs) {
        return -ENOMEM;
    }

    _fs->flash_device = flash_device;
    _fs->offset = offset;

    struct flash_pages_info info;
    int rc = flash_get_page_info_by_offs(_fs->flash_device, _fs->offset, &info);
    if (rc) {
        delete _fs;
        _fs = nullptr;
        return rc;
    }

    _fs->sector_size = info.size;
    _fs->sector_count = sector_count;

    rc = zms_mount(_fs);
    if (rc) {
        delete _fs;
        _fs = nullptr;
        return rc;
    }

    _mounted = true;
    return 0;
}

int CZmsManager::Deinit() {
    if (!_fs) {
        return -EINVAL;
    }
    int rc = zms_unmount(_fs);
    delete _fs;
    _fs = nullptr;
    _mounted = false;
    return rc;
}

static uint32_t fnv1a_32(const void *data, size_t len) {
    const uint8_t *ptr = static_cast<const uint8_t *>(data);
    uint32_t hash = 0x811c9dc5u;
    for (size_t i = 0; i < len; ++i) {
        hash ^= ptr[i];
        hash *= 0x01000193u;
    }
    return hash;
}

uint32_t CZmsManager::IdFromKey(const std::string &key) const {
    // Use FNV-1a to map strings to 32-bit ids. Avoid id 0 as some code may use it.
    uint32_t id = fnv1a_32(key.data(), key.size());
    if (id == 0) {
        id = 1;
    }
    return id;
}

ssize_t CZmsManager::Read(const std::string &key, void *buf, size_t len) {
    if (!_mounted || !_fs) {
        return -ENODEV;
    }
    uint32_t id = IdFromKey(key);
    ssize_t rc = zms_read(_fs, id, buf, len);
    return rc;
}

int CZmsManager::Write(const std::string &key, const void *data, size_t len) {
    if (!_mounted || !_fs) {
        return -ENODEV;
    }
    uint32_t id = IdFromKey(key);
    int rc = zms_write(_fs, id, data, len);
    return rc;
}

int CZmsManager::Write(const std::string &key, uint32_t value) {
    return Write(key, &value, sizeof(value));
}

int CZmsManager::Delete(const std::string &key) {
    if (!_mounted || !_fs) {
        return -ENODEV;
    }
    uint32_t id = IdFromKey(key);
    int rc = zms_delete(_fs, id);
    return rc;
}
