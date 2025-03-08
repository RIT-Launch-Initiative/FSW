#ifndef C_TFTP_SERVER_H
#define C_TFTP_SERVER_H

#include "f_core/net/network/c_ipv4.h"
#include "f_core/net/transport/c_udp_socket.h"

class CTftpServerTenant : public CTenant {
public:
    static constexpr uint16_t TFTP_DEFAULT_PORT = 69;
    inline static CTftpServerTenant *instance = nullptr;

    /**
	 * Singleton getter to avoid multiple instances of the TFTP server
	 */
    static CTftpServerTenant *getInstance(const CIPv4& ipv4, uint16_t port = TFTP_DEFAULT_PORT) {
        if (instance == nullptr) {
            instance = new CTftpServerTenant(ipv4, port);
        }

        return instance;
    }

    void Startup() override;

    void Cleanup() override;

    void Run() override;

private:
    CUdpSocket sock;

    typedef enum {
        RRQ = 1, // Read request
        WRQ = 2, // Write request
        DATA = 3, // Data
        ACK = 4, // Acknowledgement
        ERROR = 5 // Error
    } TftpOpcode;

    CTftpServerTenant(const CIPv4& ipv4, uint16_t port = TFTP_DEFAULT_PORT) : CTenant("TFTP server"), sock(ipv4, port, port) {};

    void handleReadRequest(const char *filename, const char *mode, const CIPv4& clientAddr, uint16_t clientPort);

    void handleWriteRequest(const char *filename, const char *mode, const CIPv4& clientAddr, uint16_t clientPort);

    void handleData(const char *data, size_t len, const CIPv4& clientAddr, uint16_t clientPort);

    void handleAck(uint16_t blockNum, const CIPv4& clientAddr, uint16_t clientPort);

    void handleError(uint16_t errorCode, const char *errorMsg, const CIPv4& clientAddr, uint16_t clientPort);
};



#endif //C_TFTP_SERVER_H
