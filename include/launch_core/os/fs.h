#ifndef L_LOGGER_H
#define L_LOGGER_H

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#define ENOTINIT 200 // device not initialized

#define L_FS_CREATE_FILE(name, filename, sample_width, n_samples, mode) \
    l_fs_file_t name = { \
        .fname = #filename, \
        .width = sample_width, \
        .mode = mode, \
        .size = sample_width * n_samples, \
        .initialized = false, \
        .file = {0}, \
        .dirent = {0}, \
        .vfs = {0}, \
        .wpos = 0 \
    }

typedef enum {
    SLOG_ONCE,
    SLOG_CIRC
} l_fs_log_mode_t;

typedef struct {
    const char *fname;
    const off_t width;
    const l_fs_log_mode_t mode;

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
 * @param[in] p_file 	pointer to file handle
 * @param[in] src 	buffer to write (l_fs_file_t *p_fileassumed fixed-width, with the initialized width)
 * @param[out] err_flag 	Pointer to error flag
 * @return Number of bytes written
 */
size_t l_fs_write(l_fs_file_t *p_file, const uint8_t *const src, int32_t *err_flag);

/**
 * @brief Read a frame from the device
 *
 * @param dst 	Buffer to read into (l_fs_file_t *p_fileat least as big as the frame width)
 * @param idx	Frame index to read
 */
size_t l_fs_read(l_fs_file_t *p_file, uint8_t *dst, off_t idx);

/**
 * @brief Get the file's size in bytes
 *
 * The file does not have to be open
 *
 * @retval >= 0	the file's size in bytes
 * @retval < 0	fs_stat error
 */
size_t l_fs_file_size(l_fs_file_t *p_file);

/**
 * @brief Get the available free space in bytes
 *
 * The file does not have to be open
 *
 * @retval >= 0	free space on the filesystem in bytes
 * @retval < 0	fs_statvfs error
 */
size_t l_fs_volume_free_space(l_fs_file_t *p_file);

/**
 * @brief Get the file's status
 * @param p_file File to check
 * @return 0 on success, < 0 on error
 */
int32_t l_fs_stat(l_fs_file_t *p_file);

/**
 * @brief Get the filesystem's status
 * @param p_file File to check
 * @return 0 on success, < 0 on error
 */
int32_t l_fs_stat_vfs(l_fs_file_t *p_file);

/**
 * @brief Get the boot count of the device, to see if FS can be read and written to
 * @return > 0 for boot count number, < 0 on error
 */
int32_t l_fs_boot_count_check();

/**
 * Format a partition
 * @param partition_id ID of the partition to format
 * @return Zephyr status code
 */
int32_t l_fs_format(uintptr_t partition_id);

/**
 * Create a directory
 * @param dir_name Directory name
 * @return Zephyr status code
 */
int32_t l_fs_mkdir(const char *dir_name);

#endif
