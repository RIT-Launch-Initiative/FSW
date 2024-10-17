#ifndef C_RS485_H
#define C_RS485_H

#include <f_core/net/c_transciever.h>

class CRs485 : public CTransceiver
{
public:
    CRs485();

    int TransmitSynchronous(const void* data, size_t len);

    /**
     * See parent docs
     */
    int ReceiveSynchronous(void* data, size_t len);

    /**
     * See parent docs
     */
    int TransmitAsynchronous(const void* data, size_t len);

    /**
     * See parent docs
     */
    int ReceiveAsynchronous(void* data, size_t len);

    /**
     * See parent docs
     */
    int SetRxTimeout(int timeout);
};


#endif //C_RS485_H
