#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <launch_core/os/fs.h>

LOG_MODULE_REGISTER(slog, CONFIG_LOG_DEFAULT_LEVEL);

SensorLogger::SensorLogger(
		char* fname, 
		size_t sample_width, 
		size_t n_samples,
		enum l_fs_log_mode mode): 
	fname(fname), width(sample_width), mode(mode), size(n_samples * sample_width) {};


int32_t l_fs_init(l_fs_file_t *p_file) {
	int32_t ret = 0;

	fs_file_t_init(&p_file->file);

	ret = fs_open(&p_file->file, fname, FS_O_CREATE | FS_O_RDWR);
	if (ret < 0) {
		LOG_ERR("Unable to open/create file: %d", ret);
		return ret;
	}

	if (size < width) {
		LOG_ERR("Not enough space for one frame");
		return -EDOM;
	}

	m_initalized = true;

	return ret;
}


int32_t l_fs_open(l_fs_file_t *p_file) {
	return fs_open(&p_file->file, fname, FS_O_RDWR);
}

int32_t l_fs_close(l_fs_file_t *p_file) {
	return fs_close(&p_file->file);
}


int32_t l_fs_write(l_fs_file_t *p_file, uint8_t* src) {
	int32_t ret = 0;
	if (!m_initalized) {
		LOG_ERR("Logger for file %s is not initialized", fname);
		return -ENOTINIT;
	}

	if ((m_wpos + width) > size) { // next write will go out-of-bounds
		fs_sync(&file);
		switch (mode) {
			case SLOG_ONCE: // error out
				LOG_WRN("Logger reached end of file");
				return -ENOSPC;
				break;
			case SLOG_CIRC: // wrap around
				LOG_INF("Logger reset to start of file");
				m_wpos = 0;
				break;
			default:
				return -ENOTSUP;
				break;
		}
	}

	// read/write make no garauntees about where the current seek is so m_wpos
	// keeps track of that
	ret = fs_seek(&p_file->file, m_wpos, FS_SEEK_SET);
	if (ret < 0) {
		return ret;
	}

	ret = fs_write(&p_file->file, src, width);
	if (ret < 0) {
		return ret;
	}

	m_wpos = fs_tell(&p_file->file);

	return ret;
}

int32_t l_fs_read(uint8_t* dst, size_t idx) {
	int32_t ret = 0;

	if (!m_initalized) {
		LOG_ERR("Logger for file %s is not initialized", fname);
		return -ENOTINIT;
	}

	size_t frame_start = idx * width;
	size_t frame_end = frame_start + width;

	if (frame_end > size) {
		LOG_ERR("Frame %d spans %d to %d, but max size is %d", 
				idx, frame_start, frame_end, size);
		return -EDOM; // could also be EOVERFLOW, EINVAL?
	}

	// this class makes no garauntees about where the current seek position is
	ret = fs_seek(&p_file->file, frame_start, FS_SEEK_SET);
	if (ret < 0) {
		return ret;
	}

	ret = fs_read(&p_file->file, dst, width);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

int32_t l_fs_file_size(l_fs_file_t *p_file) {
	int32_t ret = 0;
	ret = l_fs_stat(p_file);
	if (ret < 0) {
		return ret;
	}
	
	return dirent.size;
}

int32_t l_fs_volume_free_space(l_fs_file_t *p_file) {
	int32_t ret = 0;
	ret = fs_sync(&p_file->file); // otherwise, properties on disk may not reflect
							// write or truncate operations
	if (ret < 0) {
		return ret;
	}
	ret = stat_vfs();
	if (ret < 0) {
		return ret;
	}
	
	return vfs.f_bfree * vfs.f_frsize;
}

/// Internal functions

int32_t l_fs_stat(l_fs_file_t *p_file) {
	int32_t ret = fs_stat(fname, &dirent); 
	if (ret < 0) {
		LOG_ERR("Unable to stat file: %d", ret);
		return ret;
	}
	LOG_INF("%s is a %s of size %d", 
			dirent.name,
			dirent.type == FS_DIR_ENTRY_FILE ? "file" : "directory",
			dirent.size);
	return ret;
}

int32_t l_fs_stat_vfs(l_fs_file_t *p_file) {
	int32_t ret = fs_statvfs(fname, &vfs);		
	if (ret < 0) {
		LOG_ERR("Unable to stat vfs: %d", ret);
		return ret;
	}
	LOG_DBG("%s is on a volume with \r\n\t%lu blocks (%lu free) of %lu bytes each", 
				fname, 
				vfs.f_blocks, 
				vfs.f_bfree, 
				vfs.f_frsize);
	return ret;
}
