#ifndef C_TRANSCIEVER_H
#define C_TRANSCIEVER_H

#include <cstddef>

class CTransceiver {
public:
    CTransceiver() = default;

    /**
     * Transmit data (blocking)
     * @param data Buffer of data to transmit
     * @param len Size of the buffer
     * @return Zephyr status code of the transmission
     */
    virtual int TransmitSynchronous(const void *data, size_t len) = 0;

    /**
     * Receive data (blocking)
     * @param data Buffer to receive data into
     * @param len Size of the buffer
     * @return Zephyr status code of the reception
     */
    virtual int ReceiveSynchronous(void *data, size_t len) = 0;

    /**
     * Transmit data (non-blocking)
     * @param data Buffer of data to transmit
     * @param len Size of the data
     * @return Zephyr status code of the transmission
     */
    virtual int TransmitAsynchronous(const void *data, size_t len) = 0;

    /**
     * Receive data (non-blocking)
     * @param data Buffer to receive data into
     * @param len Size of the buffer
     * @return Zephyr status code of the reception
     */
    virtual int ReceiveAsynchronous(void *data, size_t len) = 0;

    /**
    * Set the timeout for transmitting asynchronously
    * @param timeoutMillis Time to wait for a transmission in milliseconds
    * @return Status code of operation
    */
    virtual int SetTxTimeout(int timeoutMillis) = 0;

    /**
     * Set the timeout for receiving asynchronously
     * @param timeoutMillis Time to wait for a reception milliseconds
     * @return Status code of operation
     */
    virtual int SetRxTimeout(int timeoutMillis) = 0;

protected:
    virtual ~CTransceiver() = default;
};



#endif //C_TRANSCIEVER_H
