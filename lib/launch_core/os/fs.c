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


int32_t l_fs_init(void) {
	int32_t ret = 0;

	fs_file_t_init(&file);

	ret = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
	if (ret < 0) {
		LOG_ERR("Unable to open/create file: %d", ret);
		return ret;
	}

	if (size < width) {
		LOG_ERR("Not enough space for one frame");
		return -EDOM;
	}

	// Idea was to pre-allocate the file using fs_truncate, but this might be a
	// very expensive operation on LittleFS.
	/* LOG_INF("Truncating file to %u bytes..", size);
	ret = fs_truncate(&file, size);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Synchronizing file...");
	ret = fs_sync(&file); // otherwise, properties on disk may not reflect
							// write or truncate operations
	if (ret < 0) {
		return ret;
	}

	ret = file_size();
	if (ret < 0) {
		return ret;
	}

	if ((size_t) ret < size) { // the file we got isn't big enough
		LOG_ERR("Could not expand to %u, got %u", size, ret);
		LOG_INF("Truncating back to zero");
		ret = fs_truncate(&file, 0);
		if (ret < 0) {
			LOG_ERR("Unable to shrink file after too-large request: %d", ret);
			return ret;
		}
		ret = fs_sync(&file);
		if (ret < 0) {
			LOG_ERR("Could not sync file: %d", ret);
			return ret;
		}

		return -ENOSPC; 
		// fs_truncate does not error on insufficient space so we have to
	} */

	m_initalized = true;

	return ret;
}


int32_t l_fs_open(void) {
	return fs_open(&file, fname, FS_O_RDWR);
}

int32_t l_fs_close(void) {
	return fs_close(&file); 
}


int32_t l_fs_write(uint8_t* src) {
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
	ret = fs_seek(&file, m_wpos, FS_SEEK_SET);
	if (ret < 0) {
		return ret;
	}

	ret = fs_write(&file, src, width);
	if (ret < 0) {
		return ret;
	}

	m_wpos = fs_tell(&file);

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
	ret = fs_seek(&file, frame_start, FS_SEEK_SET);
	if (ret < 0) {
		return ret;
	}

	ret = fs_read(&file, dst, width);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

int32_t l_fs_file_size(void) {
	int32_t ret = 0;
	ret = stat();
	if (ret < 0) {
		return ret;
	}
	
	return dirent.size;
}

int32_t l_fs_volume_free_space(void) {
	int32_t ret = 0;
	ret = fs_sync(&file); // otherwise, properties on disk may not reflect
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

int32_t l_fs_stat(void) {
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

int32_t l_fs_stat_vfs(void) {
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
