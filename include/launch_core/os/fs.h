#ifndef L_LOGGER_H
#define L_LOGGER_H

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#define ENOTINIT 200 // device not initialized

enum l_fs_log_mode {
    SLOG_ONCE,
    SLOG_CIRC
};

typedef struct {
    const char *fname;
    const size_t width;
    const enum l_fs_log_mode mode;

    const size_t size;
    bool initialized;
    struct fs_file_t file;
    struct fs_dirent dirent;
    struct fs_statvfs vfs;
    off_t wpos;
} l_fs_file_t;


/**
 * @brief Initialize the logger
 *
 * State change: File name opened in CREATE | RDWR mode
 *
 * @retval = 0: success
 * @retval < 0: other fs error
 */
int32_t l_fs_init(l_fs_file_t *p_file);

/**
 * @brief Open the target file for reading and writing
 * @retval = 0: success
 * @retval < 0: fs_open error
 */
int32_t l_fs_open(l_fs_file_t *p_file);

/**
 * @brief Close the target file
 * @retval = 0: success
 * @retval < 0: fs_close error
 */
int32_t l_fs_close(l_fs_file_t *p_file);

/**
 * @brief Seek to the appropriate (l_fs_file_t *p_filebased on the logging mode) point and write
 * the buffer
 *
 * @param src 	buffer to write (l_fs_file_t *p_fileassumed fixed-width, with the initialized width)
 *
 * @retval = 0 			success
 * @retval = -ENOTINIT 	logger was never initialized
 * @retval < 0			other errno from fs functions
 */
int32_t l_fs_write(l_fs_file_t *p_file, uint8_t *src);

/**
 * @brief Read a frame from the device
 *
 * @param dst 	Buffer to read into (l_fs_file_t *p_fileat least as big as the frame width)
 * @param idx	Frame index to read
 */
int32_t l_fs_read(l_fs_file_t *p_file, uint8_t *dst, size_t idx
);

/**
 * @brief Get the file's size in bytes
 *
 * The file does not have to be open
 *
 * @retval >= 0	the file's size in bytes
 * @retval < 0	fs_stat error
 */
int32_t l_fs_file_size(l_fs_file_t *p_file);

/**
 * @brief Get the available free space in bytes
 *
 * The file does not have to be open
 *
 * @retval >= 0	free space on the filesystem in bytes
 * @retval < 0	fs_statvfs error
 */
int32_t l_fs_volume_free_space(l_fs_file_t *p_file);

int32_t l_fs_stat(l_fs_file_t *p_file);

int32_t l_fs_stat_vfs(l_fs_file_t *p_file);

#endif
