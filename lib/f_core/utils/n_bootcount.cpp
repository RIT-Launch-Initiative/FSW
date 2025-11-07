#include "f_core/utils/n_bootcount.h"

#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(NBootCount);

static const char *bootCountFilename = "/lfs/.boot_count";

int NBootCount::GetBootCount() {
    fs_file_t bootCountFile = {0};
    uint32_t bootCount = 0;
    int32_t ret = fs_open(&bootCountFile, bootCountFilename, FS_O_READ);
    if (ret < 0) {
        LOG_ERR("Unable to open boot count file: %d", ret);
        return ret;
    }

    fs_read(&bootCountFile, &bootCount, sizeof(bootCount));
    fs_close(&bootCountFile);
    return static_cast<int>(bootCount);
}

int NBootCount::IncrementBootCount() {
    static bool bootCountIncremented = false;
    if (bootCountIncremented) {
        LOG_WRN("Boot count already checked");
        return -EALREADY;
    }
    bootCountIncremented = true;

    fs_file_t bootCountFile = {0};
    fs_mode_t flags = FS_O_RDWR;
    uint32_t bootCount = 0;

    // Check if a .boot_count file exists. If not create it
    fs_dirent ignore{};
    int32_t ret = fs_stat(bootCountFilename, &ignore);
    if (ret < 0) {
        LOG_INF("No boot count file found. Creating boot count file.");
        flags |= FS_O_CREATE;
    }

    ret = fs_open(&bootCountFile, bootCountFilename, flags);
    if (ret < 0) {
        LOG_ERR("Unable to open boot count file: %d", ret);
        return ret;
    }

    // If the file was just created, write a 0 to it
    if ((flags & FS_O_CREATE) != 0) {
        LOG_INF("Writing initial 0 to boot count file.");
        ret = fs_write(&bootCountFile, &bootCount, sizeof(bootCount));
        if (ret < 0) {
            LOG_ERR("Unable to write boot count: %d", ret);
            return ret;
        }
    }

    // Read the boot count
    ret = fs_read(&bootCountFile, &bootCount, sizeof(bootCount));
    if (ret < 0) {
        LOG_ERR("Unable to read boot count: %d", ret);
        return ret;
    }

    // Increment the boot count
    bootCount++;

    // Write the boot count
    ret = fs_seek(&bootCountFile, 0, FS_SEEK_SET);
    if (ret < 0) {
        LOG_ERR("Unable to seek to beginning of boot count file: %d", ret);
        return ret;
    }

    ret = fs_write(&bootCountFile, &bootCount, sizeof(bootCount));
    if (ret < 0) {
        LOG_ERR("Unable to write boot count: %d", ret);
        return ret;
    }

    // Close the boot count file
    ret = fs_close(&bootCountFile);
    if (ret < 0) {
        LOG_ERR("Unable to close boot count file: %d", ret);
        return ret;
    }

    LOG_INF("Boot Count: %d", boot_count);

    return static_cast<int>(bootCount);
}