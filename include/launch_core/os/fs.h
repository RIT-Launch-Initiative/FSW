#ifndef L_LOGGER_H
#define L_LOGGER_H

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#define ENOTINIT 200 // device not initialized

enum log_mode {
    SLOG_ONCE,
    SLOG_CIRC
};


/**
 * @brief Construct a logger for fixed-width samples
 *
 * @param fname 		file name to log to
 * @param sample_width 	width of one data sample
 * @param mode			SLOG_INFINITE, STOP, or CIRC, to define overflow
 * 						behavior
 * @param max_size		maximum file size in bytes
 */
SensorLogger(char *fname, size_t sample_width, size_t n_samples, enum log_mode mode);

/**
 * @brief Initialize the logger
 *
 * State change: File name opened in CREATE | RDWR mode
 *
 * @retval = 0: success
 * @retval < 0: other fs error
 */
int32_t l_fs_init(void);

/**
 * @brief Open the target file for reading and writing
 * @retval = 0: success
 * @retval < 0: fs_open error
 */
int32_t l_fs_open(void);

/**
 * @brief Close the target file
 * @retval = 0: success
 * @retval < 0: fs_close error
 */
int32_t l_fs_close(void);

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
int32_t l_fs_write(uint8_t *src);

/**
 * @brief Read a frame from the device
 *
 * @param dst 	Buffer to read into (at least as big as the frame width)
 * @param idx	Frame index to read
 */
int32_t l_fs_read(uint8_t *dst, size_t idx);

/**
 * @brief Get the file's size in bytes
 *
 * The file does not have to be open
 *
 * @retval >= 0	the file's size in bytes
 * @retval < 0	fs_stat error
 */
int32_t l_fs_file_size();

/**
 * @brief Get the available free space in bytes
 *
 * The file does not have to be open
 *
 * @retval >= 0	free space on the filesystem in bytes
 * @retval < 0	fs_statvfs error
 */
int32_t l_fs_volume_free_space();

int32_t l_fs_stat(void);

int32_t l_fs_stat_vfs(void);

const char *m_fname;
const size_t m_width;
const enum log_mode m_mode;

const size_t m_size;
bool m_initalized = false;
struct fs_file_t m_file;
struct fs_dirent m_dirent;
struct fs_statvfs m_vfs;
off_t m_wpos;

#endif
