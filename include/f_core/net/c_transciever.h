#ifndef C_TRANSCIEVER_H
#define C_TRANSCIEVER_H

class CTransceiver {
public:
    virtual int TransmitSynchronous(const void *data, size_t len);

    virtual int ReceiveSynchronous(void *data, size_t len);

    virtual int TransmitAsynchronous(const void *data, size_t len);

    virtual int ReceiveAsynchronous(void *data, size_t len);

    virtual int SetRxTimeout(int timeout);
};



#endif //C_TRANSCIEVER_H
