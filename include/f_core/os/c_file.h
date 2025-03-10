#ifndef C_FILE_H
#define C_FILE_H

#include <sys/types.h>
#include <zephyr/fs/fs_interface.h>

class CFile {
public:
    typedef enum {
        READ_FLAG = 0x01,
        WRITE_FLAG = 0x02,
        READ_WRITE_FLAG = READ_FLAG | WRITE_FLAG,
        CREATE_FLAG = 0x10,
        APPEND_FLAG = 0x20,
        TRUNC_FLAG = 0x40
    } FILE_FLAGS;

    /**
     * Constructor
     * @param path Path to the file
     * @param flags Flags for opening file
     */
    explicit CFile(const char *path, int flags);

    /**
     * Destructor
     */
    ~CFile();

    /**
     * Get the size of the file
     * @return size of the file
     */
    size_t GetFileSize() const;

    /**
     * Read contents of a file
     * @param data Buffer to read data into
     * @param len Size of the buffer
     * @param offset Offset to read from
     * @return Number of bytes read. -1 on error
     */
    int Read(void *data, size_t len, off_t offset = 0);

    /**
     * Write contents to a file
     * @param data Buffer of data to write from
     * @param len Size of the buffer
     * @return Number of bytes written. -1 on error
     */
    int Write(const void *data, off_t len);

    /**
     * Get the initialization status of the file
     * @return initStatus
     */
    int GetInitStatus() const { return initStatus; }

private:
    fs_file_t file;
    const char *path;
    const int flags;
    int initStatus = -1;
};

#endif //C_FILE_H
