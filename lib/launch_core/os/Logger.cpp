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
		size_t max_size,
		enum log_mode mode): 
	m_fname(fname), m_width(sample_width), m_size(max_size), m_mode(mode) {};


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

	stat();
	stat_vfs();

	// size_t size = fs_truncate(&m_file, m_size);
	// if (size != m_size) { // couldn't get enough space
	// 	return -ENOSPC;
	// }

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
		LOG_ERR("Logger for file %s is not initialized", m_fname);
		return -ENOTINIT;
	}

	if ((m_wpos + m_width) >= m_size) { // next write will go out-of-bounds
		switch (m_mode) {
			case SLOG_ONCE: // error out
				LOG_INF("Logger reached end of file");
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
		LOG_ERR("Frame index %d spans %d to %d, but max size is %d", 
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
 * @brief Stat the file
 *
 */
int32_t SensorLogger::stat(void) {
	int32_t ret = fs_stat(m_fname, &m_dirent); 
	LOG_DBG("%s is a %s of size %d", 
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
	LOG_DBG("%s is on a volume with \r\n\t%lu blocks (%lu free) of %lu bytes each", 
				m_fname, 
				m_vfs.f_blocks, 
				m_vfs.f_bfree, 
				m_vfs.f_frsize);
	return ret;
}
