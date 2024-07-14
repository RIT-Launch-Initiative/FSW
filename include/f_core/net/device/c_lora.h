#ifndef C_LORA_H
#define C_LORA_H

#include <f_core/net/c_transciever.h>

struct device;

class CLora : public CTransceiver
{
public:
    CLora(const device* lora_dev);

    int TransmitSynchronous(const void* data, size_t len);

    int ReceiveSynchronous(void* data, size_t len);

    int TransmitAsynchronous(const void* data, size_t len);

    int ReceiveAsynchronous(void* data, size_t len);

    int SetRxTimeout(int timeout);

protected:
    ~CLora() = default;

private:
    const device* lora_dev;
};


#endif //C_LORA_H
