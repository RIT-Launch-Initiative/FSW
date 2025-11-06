#include "f_core/os/c_zms_manager.h"

#include <zephyr/fs/zms.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CZmsManager);

static uint32_t fnv1a_32(const void *data, size_t len) {
    const uint8_t *ptr = static_cast<const uint8_t *>(data);
    uint32_t hash = 0x811c9dc5U;
    for (size_t i = 0; i < len; ++i) {
        hash ^= ptr[i];
        hash *= 0x01000193U;
    }
    return hash;
}

CZmsManager::CZmsManager(device &flash, off_t offset, uint32_t sectorCount) : mounted(false) {
    fs.flash_device = &flash;
    fs.offset = offset;

    flash_pages_info info;
    int ret = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
    if (ret != 0) {
        LOG_ERR("Failed to get flash page info: %d", ret);
        return;
    }

    fs.sector_size = info.size;
    fs.sector_count = sectorCount;

    ret = zms_mount(&fs);
    if (ret != 0) {
        LOG_ERR("Failed to mount ZMS: %d", ret);
        return;
    }

    uint32_t bootCount = 0;
    ssize_t readResult = zms_read(&fs, IdFromKey("bootcount"), &bootCount, sizeof(bootCount));
    if (readResult >= 0 && static_cast<size_t>(readResult) == sizeof(bootCount)) {
        bootCount++;
    } else {
        bootCount = 1;
    }
    int writeResult = zms_write(&fs, IdFromKey("bootcount"), &bootCount, sizeof(bootCount));
    if (writeResult < 0) {
        LOG_ERR("Failed to write bootcount: %d", writeResult);
    } else {
        LOG_INF("Bootcount: %u", bootCount);
    }

    mounted = true;
}

uint32_t CZmsManager::IdFromKey(const std::string &key) const {
    uint32_t id = fnv1a_32(key.data(), key.size());
    if (id == 0) {
        id = 1;
    }
    return id;
}

ssize_t CZmsManager::Read(const std::string &key, void *buf, size_t len) {
    if (!mounted) {
        return -ENODEV;
    }

    const uint32_t id = IdFromKey(key);
    return zms_read(&fs, id, buf, len);
}

int CZmsManager::Write(const std::string &key, const void *data, size_t len) {
    if (!mounted) {
        return -ENODEV;
    }

    const uint32_t id = IdFromKey(key);
    return zms_write(&fs, id, data, len);
}

int CZmsManager::Write(const std::string &key, uint32_t value) {
    return Write(key, &value, sizeof(value));
}

int CZmsManager::Delete(const std::string &key) {
    if (!mounted) {
        return -ENODEV;
    }
    const uint32_t id = IdFromKey(key);
    return zms_delete(&fs, id);
}
