#include "zephyr/logging/log.h"
#include <launch_core/os/Logger.h>
LOG_MODULE_REGISTER(slog, CONFIG_LOG_DEFAULT_LEVEL);

/**
 * @brief Construct a logger for fixed-width samples
 * 
 * @param fname 		file name to log to
 * @param sample_width 	width of one data sample
 * @param mode			SLOG_INFINITE, STOP, or CIRC, to define overflow
 * 						behavior
 */
SensorLogger::SensorLogger(
		char* fname, 
		size_t sample_width, 
		enum log_mode mode, 
		size_t max_size): 
	m_fname(fname), m_width(sample_width), m_mode(mode), m_size(max_size) 
{};


/**
 * @brief Initialize the logger
 *
 * State change: File name opened in CREATE | RDWR mode
 *
 * State change: File expanded to max_size;
 *
 * @retval = 0: success
 * @retval = -200: check failed
 * @retval < 0: other fs error
 */
int32_t SensorLogger::init(void) {
	int32_t ret = 0;

	fs_file_t_init(&m_file);

	ret = fs_open(&m_file, m_fname, FS_O_CREATE | FS_O_RDWR);
	if (ret < 0) {
		return ret;
	}

	stat();
	stat_vfs();

	if (m_size != 0) {
		size_t size = fs_truncate(&m_file, m_size);
		if (size != m_size) { // couldn't get enough space
			return -ENOSPC;
		}
	}

	m_initalized = true;

	return ret;
}


/**
 * @brief Seek to the appropriate (based on the logging mode) point and write
 * the buffer
 *
 * @param src 	buffer to write (assumed fixed-width, with the initialized width)
 */
int32_t SensorLogger::write(uint8_t* src) {
	int32_t ret = 0;
	if (!m_initalized) {
		return -ENOTINIT;
	}

	switch (m_mode) {
		case SLOG_INFINITE:
			break;
		case SLOG_STOP:
			if ((m_wpos + m_width) > m_size) {
				ret = -ENOSPC;
			}
			break;	
		case SLOG_CIRC:
			if ((m_wpos + m_width) >= m_size) {
				m_wpos = 0;
			}
			ret = -ENOTSUP;
			break;
	}

	if (ret < 0) {
		return ret;
	}

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

int32_t SensorLogger::read(size_t idx, uint8_t* dst) {
	int32_t ret = 0;
	if (!m_initalized) {
		return -ENOTINIT;
	}

	ret = fs_seek(&m_file, idx * m_width, FS_SEEK_SET);
	if (ret < 0) {
		return ret;
	}

	ret = fs_read(&m_file, dst, m_width);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

int32_t SensorLogger::stat(void) {
	int32_t ret = fs_stat(m_fname, &m_dirent); 
	LOG_DBG("%s is a %s of size %d", 
			m_dirent.name,
			m_dirent.type == FS_DIR_ENTRY_FILE ? "file" : "directory",
			m_dirent.size);
	return ret;
}

int32_t SensorLogger::stat_vfs(void) {
	int32_t ret = fs_statvfs(m_fname, &m_vfs);		
	LOG_DBG("%s is on a volume with \r\n\t%lu blocks (%lu free) of %lu bytes each", 
				m_fname, m_vfs.f_blocks, m_vfs.f_bfree, m_vfs.f_frsize);
	return ret;
}
