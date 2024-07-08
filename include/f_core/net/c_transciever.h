#ifndef C_TRANSCIEVER_H
#define C_TRANSCIEVER_H

#include <cstddef>

class CTransceiver {
public:
    CTransceiver() = default;

    virtual int TransmitSynchronous(const void *data, size_t len) = 0;

    virtual int ReceiveSynchronous(void *data, size_t len) = 0;

    virtual int TransmitAsynchronous(const void *data, size_t len) = 0;

    virtual int ReceiveAsynchronous(void *data, size_t len) = 0;

    virtual int SetRxTimeout(int timeout) = 0;
protected:
    ~CTransceiver() = default;
};



#endif //C_TRANSCIEVER_H
