
#ifndef LOGGER_H
#define LOGGER_H
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#define ENOTINIT 200

/** FS_O_READ open for read
 * FS_O_WRITE open for write
 * FS_O_RDWR open for read/write (FS_O_READ | FS_O_WRITE)
 * FS_O_CREATE create file if it does not exist
 * FS_O_APPEND move to end of file before each write
 */

enum log_mode {
	SLOG_INFINITE,
	SLOG_STOP,
	SLOG_CIRC
};

class SensorLogger {
public:

	SensorLogger(char* fname, size_t sample_width, enum log_mode mode, size_t max_size = 0);

	int32_t init(void);

	int32_t write(uint8_t* src);

	int32_t read(size_t idx, uint8_t* dst);

protected:
	int32_t stat(void);
	int32_t stat_vfs(void);

	// constructed with these members
	bool m_initalized = false;
	char* m_fname;
	size_t m_width;
	enum log_mode m_mode;
	size_t m_size;

	// runtime variables
	struct fs_file_t m_file;
	struct fs_dirent m_dirent;
	struct fs_statvfs m_vfs;
	off_t m_wpos;
};

#endif
