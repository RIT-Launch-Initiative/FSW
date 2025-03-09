#ifndef C_FILE_H
#define C_FILE_H
#include <zephyr/fs/fs_interface.h>

class CFile {
public:
    explicit CFile(const char *path, int flags);

    ~CFile();

    size_t GetFileSize() const;

    int Read(void *data, size_t len);

    int Write(const void *data, size_t len);

private:
    fs_file_t file;
    const char* path;
    const int flags;
    int initStatus = -1;
};



#endif //C_FILE_H
