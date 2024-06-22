#include <launch_core_classic/os/fs.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(l_fs, CONFIG_LOG_DEFAULT_LEVEL);

int32_t l_fs_init(l_fs_file_t *p_file) {
    fs_file_t_init(&p_file->file);

    int32_t ret = fs_open(&p_file->file, p_file->fname, FS_O_CREATE | FS_O_RDWR);
    if (ret < 0) {
        LOG_ERR("Unable to open/create file: %d", ret);
        return ret;
    }

    if (p_file->size < p_file->width) {
        LOG_ERR("Not enough space for one frame");
        return -EDOM;
    }

    p_file->initialized = true;

    return 0;
}

int32_t l_fs_open(l_fs_file_t *p_file) { return fs_open(&p_file->file, p_file->fname, FS_O_RDWR); }

int32_t l_fs_close(l_fs_file_t *p_file) { return fs_close(&p_file->file); }

size_t l_fs_write(l_fs_file_t *p_file, const uint8_t *const src, int32_t *err_flag) {
    if (!p_file->initialized) {
        LOG_ERR("Logger for file %s is not initialized", p_file->fname);
        *err_flag = -ENOTINIT;
        return 0;
    }

    if ((p_file->wpos + p_file->width) > p_file->size) { // next write will go out-of-bounds
        fs_sync(&p_file->file);
        switch (p_file->mode) {
            case SLOG_ONCE: // error out
                LOG_WRN("Logger reached end of file");
                *err_flag = -ENOSPC;
                return 0;
            case SLOG_CIRC: // wrap around
                LOG_INF("Logger reset to start of file");
                p_file->wpos = 0;
                break;
            default:
                *err_flag = -ENOTSUP;
        }
    }

    // read/write make no guarantees about where the current seek is so p_file->wpos
    // keeps track of that
    int32_t ret = fs_seek(&p_file->file, p_file->wpos, FS_SEEK_SET);
    if (ret < 0) {
        return ret;
    }

    ret = fs_write(&p_file->file, src, p_file->width);
    if (ret < 0) {
        return ret;
    }

    p_file->wpos = fs_tell(&p_file->file);

    return ret;
}

size_t l_fs_read(l_fs_file_t *p_file, uint8_t *dst, off_t idx) {
    if (!p_file->initialized) {
        LOG_ERR("Logger for file %s is not initialized", p_file->fname);
        return -ENOTINIT;
    }

    off_t frame_start = idx * p_file->width;
    off_t frame_end = frame_start + p_file->width;

    if (frame_end > p_file->size) {
        // LOG_ERR("Frame %ld spans %ld to %ld, but max size is %lld", idx, frame_start, frame_end, p_file->size);
        return -EDOM; // could also be EOVERFLOW, EINVAL?
    }

    // this class makes no guarantees about where the current seek position is
    int32_t ret = fs_seek(&p_file->file, frame_start, FS_SEEK_SET);
    if (ret < 0) {
        return ret;
    }

    return fs_read(&p_file->file, dst, p_file->width);
}

size_t l_fs_file_size(l_fs_file_t *p_file) {
    int32_t ret = l_fs_stat(p_file);
    if (ret < 0) {
        LOG_ERR("Unable to stat file: %d", ret);
        return ret;
    }

    return p_file->dirent.size;
}

size_t l_fs_volume_free_space(l_fs_file_t *p_file) {
    int32_t ret = fs_sync(&p_file->file); // otherwise, properties on disk may not reflect

    // write or truncate operations
    if (ret < 0) {
        LOG_ERR("Unable to sync file: %d", ret);
        return ret;
    }

    ret = l_fs_stat_vfs(p_file);
    if (ret < 0) {
        LOG_ERR("Unable to stat vfs: %d", ret);
        return ret;
    }

    return p_file->vfs.f_bfree * p_file->vfs.f_frsize;
}

int32_t l_fs_stat(l_fs_file_t *p_file) {
    int32_t ret = fs_stat(p_file->fname, &p_file->dirent);
    if (ret < 0) {
        LOG_ERR("Unable to stat file: %d", ret);
        return ret;
    }

    LOG_INF("%s is a %s of size %d", p_file->dirent.name,
            p_file->dirent.type == FS_DIR_ENTRY_FILE ? "file" : "directory", p_file->dirent.size);

    return ret;
}

int32_t l_fs_stat_vfs(l_fs_file_t *p_file) {
    int32_t ret = fs_statvfs(p_file->fname, &p_file->vfs);
    if (ret < 0) {
        LOG_ERR("Unable to stat vfs: %d", ret);
        return ret;
    }

    LOG_DBG("%s is on a volume with \r\n\t%lu blocks (%lu free) of %lu bytes each", p_file->fname, p_file->vfs.f_blocks,
            p_file->vfs.f_bfree, p_file->vfs.f_frsize);

    return ret;
}

int32_t l_fs_boot_count_check() {
    static bool boot_count_checked = false;
    if (boot_count_checked) {
        LOG_WRN("Boot count already checked");
        return -EALREADY;
    }
    boot_count_checked = true;

    static const char *boot_count_fname = "/lfs/.boot_count";
    struct fs_file_t boot_count_file = {0};
    fs_mode_t flags = FS_O_RDWR;
    uint32_t boot_count = 0;

    // Check if a .boot_count file exists. If not create it
    struct fs_dirent ignore;
    int32_t ret = fs_stat(boot_count_fname, &ignore);
    if (ret < 0) {
        LOG_INF("No boot count file found. Creating boot count file.");
        flags |= FS_O_CREATE;
    }

    ret = fs_open(&boot_count_file, boot_count_fname, flags);
    if (ret < 0) {
        LOG_ERR("Unable to open boot count file: %d", ret);
        return ret;
    }

    // If the file was just created, write a 0 to it
    if (flags & FS_O_CREATE) {
        LOG_INF("Writing initial 0 to boot count file.");
        ret = fs_write(&boot_count_file, &boot_count, sizeof(boot_count));
        if (ret < 0) {
            LOG_ERR("Unable to write boot count: %d", ret);
            return ret;
        }
    }

    // Read the boot count
    ret = fs_read(&boot_count_file, &boot_count, sizeof(boot_count));
    if (ret < 0) {
        LOG_ERR("Unable to read boot count: %d", ret);
        return ret;
    }

    // Increment the boot count
    boot_count++;

    // Write the boot count
    ret = fs_seek(&boot_count_file, 0, FS_SEEK_SET);
    if (ret < 0) {
        LOG_ERR("Unable to seek to beginning of boot count file: %d", ret);
        return ret;
    }

    ret = fs_write(&boot_count_file, &boot_count, sizeof(boot_count));
    if (ret < 0) {
        LOG_ERR("Unable to write boot count: %d", ret);
        return ret;
    }

    // Close the boot count file
    ret = fs_close(&boot_count_file);
    if (ret < 0) {
        LOG_ERR("Unable to close boot count file: %d", ret);
        return ret;
    }

    LOG_INF("Boot Count: %d", boot_count);

    return boot_count;
}

int32_t l_fs_format(uintptr_t partition_id) {
    int ret = fs_mkfs(FS_LITTLEFS, partition_id, NULL, 0);

    if (ret < 0) {
        LOG_ERR("Format failed with error %d", ret);
        return ret;
    }

    LOG_INF("Format successful");
    return 0;
}
