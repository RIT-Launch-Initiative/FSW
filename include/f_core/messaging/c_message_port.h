#ifndef CMESSAGEPORT_H
#define CMESSAGEPORT_H

template <typename T>
class CMessagePort {
public:
    CMessagePort() = default;
    virtual ~CMessagePort() = default;

    virtual void Send(T message) = 0;

    virtual T Receive() = 0;
};



#endif //CMESSAGEPORT_H
