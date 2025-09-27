#ifndef C_RAW_DATALOGGER_H
#define C_RAW_DATALOGGER_H

#include <zephyr/storage/stream_flash.h>

template <typename T, size_t bufferSize>
class CRawDataLogger {
public:
    CRawDataLogger(const device &flash) : flash(flash), ctx(nullptr) {}

    int Write() {

    }

    int Close() {

    }

    int Flush() {

    }


private:
    const device& flash;
    stream_flash_ctx *ctx;
    uint8_t buffer[bufferSize];
};

#endif //C_RAW_DATALOGGER_H