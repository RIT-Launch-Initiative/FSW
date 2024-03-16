#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <launch_core/os/Logger.h>

LOG_MODULE_REGISTER(slog, CONFIG_LOG_DEFAULT_LEVEL);

/**
 * @brief Construct a logger for fixed-width samples
 * 
 * @param fname 		file name to log to
 * @param sample_width 	width of one data sample
 * @param mode			SLOG_INFINITE, STOP, or CIRC, to define overflow
 * 						behavior
 * @param max_size		maximum file size in bytes
 */
SensorLogger::SensorLogger(
		char* fname, 
		size_t sample_width, 
		size_t n_samples,
		enum log_mode mode): 
	m_fname(fname), m_width(sample_width), m_mode(mode), m_size(n_samples * sample_width) {};


/**
 * @brief Initialize the logger
 *
 * State change: File name opened in CREATE | RDWR mode
 *
 * @retval = 0: success
 * @retval < 0: other fs error
 */
int32_t SensorLogger::init(void) {
	int32_t ret = 0;

	fs_file_t_init(&m_file);

	ret = fs_open(&m_file, m_fname, FS_O_CREATE | FS_O_RDWR);
	if (ret < 0) {
		LOG_ERR("Unable to open/create file: %d", ret);
		return ret;
	}

	if (m_size < m_width) {
		LOG_ERR("Not enough space for one frame");
		return -EDOM;
	}

	// Idea was to pre-allocate the file using fs_truncate, but this might be a
	// very expensive operation on LittleFS.
	/* LOG_INF("Truncating file to %u bytes..", m_size);
	ret = fs_truncate(&m_file, m_size);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Synchronizing file...");
	ret = fs_sync(&m_file); // otherwise, properties on disk may not reflect
							// write or truncate operations
	if (ret < 0) {
		return ret;
	}

	ret = file_size();
	if (ret < 0) {
		return ret;
	}

	if ((size_t) ret < m_size) { // the file we got isn't big enough
		LOG_ERR("Could not expand to %u, got %u", m_size, ret);
		LOG_INF("Truncating back to zero");
		ret = fs_truncate(&m_file, 0);
		if (ret < 0) {
			LOG_ERR("Unable to shrink file after too-large request: %d", ret);
			return ret;
		}
		ret = fs_sync(&m_file);
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

/**
 * @brief Open the target file for reading and writing
 * @retval = 0: success
 * @retval < 0: fs_open error
 */

int32_t SensorLogger::open(void) {
	return fs_open(&m_file, m_fname, FS_O_RDWR);
}

/**
 * @brief Close the target file
 * @retval = 0: success
 * @retval < 0: fs_close error
 */
int32_t SensorLogger::close(void) {
	return fs_close(&m_file); 
}


/**
 * @brief Seek to the appropriate (based on the logging mode) point and write
 * the buffer
 *
 * @param src 	buffer to write (assumed fixed-width, with the initialized width)
 *
 * @retval = 0 			success
 * @retval = -ENOTINIT 	logger was never initialized
 * @retval < 0			other errno from fs functions
 */
int32_t SensorLogger::write(uint8_t* src) {
	int32_t ret = 0;
	if (!m_initalized) {
		LOG_ERR("Logger for file %s is not initialized", m_fname);
		return -ENOTINIT;
	}

	if ((m_wpos + m_width) > m_size) { // next write will go out-of-bounds
		fs_sync(&m_file);
		switch (m_mode) {
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
	ret = fs_seek(&m_file, m_wpos, FS_SEEK_SET);
	if (ret < 0) {
		return ret;
	}

	ret = fs_write(&m_file, src, m_width);
	if (ret < 0) {
		return ret;
	}

	m_wpos = fs_tell(&m_file);

	return ret;
}

/**
 * @brief Read a frame from the device
 *
 * @param dst 	Buffer to read into (at least as big as the frame width)
 * @param idx	Frame index to read
 *
 * I'm not sure whether the arguments "should" be the other way around
 */
int32_t SensorLogger::read(uint8_t* dst, size_t idx) { 
	int32_t ret = 0;

	if (!m_initalized) {
		LOG_ERR("Logger for file %s is not initialized", m_fname);
		return -ENOTINIT;
	}

	size_t frame_start = idx * m_width;
	size_t frame_end = frame_start + m_width;

	if (frame_end > m_size) {
		LOG_ERR("Frame %d spans %d to %d, but max size is %d", 
				idx, frame_start, frame_end, m_size);
		return -EDOM; // could also be EOVERFLOW, EINVAL?
	}

	// this class makes no garauntees about where the current seek position is
	ret = fs_seek(&m_file, frame_start, FS_SEEK_SET);
	if (ret < 0) {
		return ret;
	}

	ret = fs_read(&m_file, dst, m_width);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

/**
 * @brief Get the file's size in bytes
 *
 * The file does not have to be open
 *
 * @retval >= 0	the file's size in bytes
 * @retval < 0	fs_stat error
 */
int32_t SensorLogger::file_size(void) {
	int32_t ret = 0;
	ret = stat();
	if (ret < 0) {
		return ret;
	}
	
	return m_dirent.size;
}

/**
 * @brief Get the available free space in bytes
 *
 * The file does not have to be open
 *
 * @retval >= 0	free space on the filesystem in bytes
 * @retval < 0	fs_statvfs error
 */
int32_t SensorLogger::volume_free_space(void) {
	int32_t ret = 0;
	ret = fs_sync(&m_file); // otherwise, properties on disk may not reflect
							// write or truncate operations
	if (ret < 0) {
		return ret;
	}
	ret = stat_vfs();
	if (ret < 0) {
		return ret;
	}
	
	return m_vfs.f_bfree * m_vfs.f_frsize;
}

/// Internal functions

/**
 * @brief Stat the file
 *
 */
int32_t SensorLogger::stat(void) {
	int32_t ret = fs_stat(m_fname, &m_dirent); 
	if (ret < 0) {
		LOG_ERR("Unable to stat file: %d", ret);
		return ret;
	}
	LOG_INF("%s is a %s of size %d", 
			m_dirent.name,
			m_dirent.type == FS_DIR_ENTRY_FILE ? "file" : "directory",
			m_dirent.size);
	return ret;
}

/**
 * @brief Stat the volume the file is on
 */
int32_t SensorLogger::stat_vfs(void) {
	int32_t ret = fs_statvfs(m_fname, &m_vfs);		
	if (ret < 0) {
		LOG_ERR("Unable to stat vfs: %d", ret);
		return ret;
	}
	LOG_DBG("%s is on a volume with \r\n\t%lu blocks (%lu free) of %lu bytes each", 
				m_fname, 
				m_vfs.f_blocks, 
				m_vfs.f_bfree, 
				m_vfs.f_frsize);
	return ret;
}
