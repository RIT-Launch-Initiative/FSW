#ifndef C_TRANSCIEVER_H
#define C_TRANSCIEVER_H

#include <cstddef>
#include <zephyr/drivers/lora.h>

class CTransceiver {
public:
    /**
     * Constructor
     */
    CTransceiver() = default;

    /**
     * Transmit data
     * @param data[in] Data to transmit
     * @param len[in] Size of data
     */
    virtual int Transmit(const void *data, size_t len) = 0;

    /**
    * Receive data
    * @param data[out] Buffer to receive data into
    * @param len[out] Size of buffer for receiving data
    */
    virtual int Receive(void *data, size_t len) = 0;

protected:
    ~CTransceiver() = default;
};



#endif //C_TRANSCIEVER_H
