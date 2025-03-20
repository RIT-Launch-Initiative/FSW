#include "f_core/os/c_file.h"

#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/parser.h>

LOG_MODULE_REGISTER(CFile);

CFile::CFile(const char* path, const int flags) : path(path), flags(flags) {
    fs_file_t_init(&file);

    initStatus = fs_open(&file, path, flags);

    if (initStatus < 0) {
        LOG_ERR("Error opening %s. %d", path, initStatus);
    }
}

CFile::~CFile() {
    if (initStatus == 0) {
        fs_close(&file);
    }
}

size_t CFile::GetFileSize() const {
    fs_dirent entry;

    if (const int err = fs_stat(path, &entry); err < 0) {
        LOG_ERR("Error getting file size: %d", err);
        return 0; // size_t is unsigned, so return 0 on error
    };

    return entry.size;
}

int CFile::Read(void* data, const size_t len, off_t offset) {
    if (initStatus < 0) {
        LOG_ERR("File not initialized");
        return -1;
    } else if (!(flags & FS_O_READ)) {
        LOG_ERR("File not opened for reading");
        return -1;
    }

    // Seek to offset
    if (offset > 0) {
        if (fs_seek(&file, offset, FS_SEEK_SET) < 0) {
            LOG_ERR("Error seeking file");
            return -1;
        }
    }

    return fs_read(&file, data, len);
}

int CFile::Write(const void* data, const size_t len) {
    if (initStatus < 0) {
        LOG_ERR("File not initialized");
        return -1;
    } else if (!(flags & FS_O_WRITE)) {
        LOG_ERR("File not opened for writing");
        return -1;
    }

    return fs_write(&file, data, len);
}


