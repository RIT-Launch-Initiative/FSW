#ifndef CMESSAGEPORT_H
#define CMESSAGEPORT_H

template <typename T>
class CMessagePort {
public:
    CMessagePort() = default;
    virtual ~CMessagePort() = default;

    virtual int Send(const T &message, const k_timeout_t timeout = K_FOREVER) = 0;

    virtual int Receive(T message, const k_timeout_t timeout = K_FOREVER) = 0;
};



#endif //CMESSAGEPORT_H
