#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <launch_core/os/fs.h>

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


int32_t l_fs_open(l_fs_file_t *p_file) {
    return fs_open(&p_file->file, p_file->fname, FS_O_RDWR);
}

int32_t l_fs_close(l_fs_file_t *p_file) {
    return fs_close(&p_file->file);
}

size_t l_fs_write(l_fs_file_t *p_file, const uint8_t *const src) {
    if (!p_file->initialized) {
        LOG_ERR("Logger for file %s is not initialized", p_file->fname);
        return -ENOTINIT;
    }

    if ((p_file->wpos + p_file->width) > p_file->size) { // next write will go out-of-bounds
        fs_sync(&p_file->file);
        switch (p_file->mode) {
            case SLOG_ONCE: // error out
                LOG_WRN("Logger reached end of file");
                return -ENOSPC;
            case SLOG_CIRC: // wrap around
                LOG_INF("Logger reset to start of file");
                p_file->wpos = 0;
                break;
            default:
                return -ENOTSUP;
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
        LOG_ERR("Frame %d spans %d to %d, but max size is %d",
                idx, frame_start, frame_end, p_file->size);
        return -EDOM; // could also be EOVERFLOW, EINVAL?
    }

    // this class makes no garauntees about where the current seek position is
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

    LOG_INF("%s is a %s of size %d",
            p_file->dirent.name,
            p_file->dirent.type == FS_DIR_ENTRY_FILE ? "file" : "directory",
            p_file->dirent.size);

    return ret;
}

int32_t l_fs_stat_vfs(l_fs_file_t *p_file) {
    int32_t ret = fs_statvfs(p_file->fname, &p_file->vfs);
    if (ret < 0) {
        LOG_ERR("Unable to stat vfs: %d", ret);
        return ret;
    }

    LOG_DBG("%s is on a volume with \r\n\t%lu blocks (%lu free) of %lu bytes each",
            p_file->fname,
            p_file->vfs.f_blocks,
            p_file->vfs.f_bfree,
            p_file->vfs.f_frsize);

    return ret;
}
