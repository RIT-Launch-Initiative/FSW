#ifndef C_FILE_H
#define C_FILE_H

#include <sys/types.h>
#include <zephyr/fs/fs_interface.h>

class CFile {
public:
    explicit CFile(const char *path, int flags);

    ~CFile();

    size_t GetFileSize() const;

    int Read(void *data, size_t len, off_t offset = 0);

    int Write(const void *data, size_t len);

    int GetStatus() const { return initStatus; }

private:
    fs_file_t file;
    const char* path;
    const int flags;
    int initStatus = -1;
};



#endif //C_FILE_H
