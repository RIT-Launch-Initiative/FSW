#ifndef C_TRANSCIEVER_H
#define C_TRANSCIEVER_H

#include <cstddef>
#include <zephyr/drivers/lora.h>

class CTransceiver {
public:
    CTransceiver()   = default;

    virtual int Transmit(const void *data, size_t len) = 0;

    virtual int Receive(void *data, size_t len) = 0;

protected:
    ~CTransceiver() = default;
};



#endif //C_TRANSCIEVER_H
